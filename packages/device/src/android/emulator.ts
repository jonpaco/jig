import { spawn } from 'child_process';
import { resolveSDKTool, resolveAbi, findBestSystemImage, findSystemImage } from './sdk';
import { createAvd, deleteAvd, getExistingAvdAbi } from './avd';
import { execFileAsync, sleep } from '../util';
import { info, debug, timed } from '../log';
import { AndroidProfile, DeviceHandle } from '../types';

/**
 * List connected Android devices/emulators in "device" state.
 */
export async function getConnectedDevices(): Promise<string[]> {
  try {
    const { stdout } = await execFileAsync('adb', ['devices']);
    return stdout
      .split('\n')
      .slice(1)
      .map((line) => line.trim())
      .filter((line) => line.endsWith('\tdevice'))
      .map((line) => line.split('\t')[0]);
  } catch {
    return [];
  }
}

/**
 * Boot an Android emulator from a profile.
 */
export async function bootAndroid(
  profileName: string,
  profile: AndroidProfile,
  timeout: number = 120_000,
): Promise<DeviceHandle> {
  const abi = resolveAbi(profile.arch);
  debug(`Profile "${profileName}": api=${profile.api}, abi=${abi}, image=${profile.image}, ram=${profile.ram}, headless=${profile.headless}`);

  // Find system image
  let image = await timed('Find system image', () => findSystemImage(profile.api, profile.image, abi));
  if (!image) {
    debug(`Exact image not found, searching for best match for ${abi}`);
    image = await findBestSystemImage(abi);
  }
  if (!image) {
    throw new Error(
      `No Android system image found for ${abi}.\n` +
      `Install one with:\n` +
      `  sdkmanager "system-images;android-${profile.api};${profile.image};${abi}"`,
    );
  }
  debug(`Using system image: ${image}`);

  // Check existing AVD ABI — delete and recreate if mismatched
  const existingAbi = getExistingAvdAbi(profileName);
  if (existingAbi && existingAbi !== abi) {
    debug(`Existing AVD has ABI ${existingAbi}, need ${abi} — deleting`);
    deleteAvd(profileName);
  }

  // Create AVD
  const { avdName } = createAvd({
    profileName,
    systemImage: image,
    ram: profile.ram,
    screen: profile.screen,
  });
  info(`AVD ${avdName} created`);

  // Boot
  const serial = await timed('Boot emulator', () => spawnEmulator(avdName, profile.headless, timeout));

  // Post-boot setup
  await timed('Wait for boot', () => waitForBoot(serial, timeout));
  await disableAnimations(serial);

  info(`Device ready (${serial})`);
  return {
    id: serial,
    platform: 'android',
    name: profileName,
    port: parseInt(serial.replace('emulator-', ''), 10),
  };
}

/**
 * Kill an Android emulator by serial.
 */
export async function killAndroid(serial: string): Promise<void> {
  try {
    await execFileAsync('adb', ['-s', serial, 'emu', 'kill'], { timeout: 10_000 });
  } catch {
    // Best effort — emulator may already be dead
  }
}

async function spawnEmulator(
  avdName: string,
  headless: boolean,
  timeout: number,
): Promise<string> {
  const emulatorBin = resolveSDKTool('emulator', true);

  // Platform-specific flags:
  // Linux: -no-window is safe (headless binary has KVM on Linux)
  // macOS: do NOT use -no-window (headless binary lacks HVF entitlement)
  const isLinux = process.platform === 'linux';
  const args = [
    '-avd', avdName,
    '-no-audio',
    '-no-boot-anim',
    '-gpu', isLinux ? 'swiftshader_indirect' : 'auto',
    ...(isLinux && headless ? ['-no-window'] : []),
  ];

  debug(`Spawning: ${emulatorBin} ${args.join(' ')}`);

  const proc = spawn(emulatorBin, args, {
    detached: true,
    stdio: ['ignore', 'ignore', 'pipe'],
    env: process.env,
  });

  // Log emulator stderr for diagnostics
  if (proc.stderr) {
    proc.stderr.on('data', (data: Buffer) => {
      const line = data.toString().trim();
      if (line.includes('hvf') || line.includes('HVF') ||
          line.includes('error') || line.includes('ERROR') ||
          line.includes('Failed') || line.includes('WARNING')) {
        debug(`[emulator] ${line}`);
      }
    });
  }

  proc.unref();

  // Wait for adb to see the emulator
  const deadline = Date.now() + timeout;
  let lastLog = 0;
  while (Date.now() < deadline) {
    try {
      const { stdout } = await execFileAsync('adb', ['devices']);
      const lines = stdout.split('\n').slice(1);
      for (const line of lines) {
        const parts = line.trim().split('\t');
        if (parts[0]?.startsWith('emulator-') && parts[1]) {
          info(`Emulator detected: ${parts[0]} (${parts[1]})`);
          if (proc.stderr) proc.stderr.destroy();
          return parts[0];
        }
      }
    } catch {
      // adb not ready yet
    }
    const elapsed = Date.now() - (deadline - timeout);
    if (elapsed - lastLog >= 15_000) {
      info(`Waiting for emulator... (${Math.round(elapsed / 1000)}s)`);
      lastLog = elapsed;
    }
    await sleep(2000);
  }

  throw new Error(
    `Emulator started but not detected by adb within ${timeout / 1000}s.\n` +
    `Check that the Android emulator is installed:\n` +
    `  sdkmanager "emulator"`,
  );
}

/**
 * On-device readiness script. Pushed once to /data/local/tmp/ and polled
 * with a single `adb shell` call per iteration. Checks all boot signals
 * in one round-trip instead of multiple adb calls.
 *
 * Exit codes:
 *   0 = ready (all checks pass)
 *   1 = not ready yet (outputs which check failed)
 */
const READY_SCRIPT = `#!/system/bin/sh
# jig-ready.sh — device readiness check

# 1. Kernel boot complete
boot=$(getprop sys.boot_completed)
if [ "$boot" != "1" ]; then
  echo "WAIT boot_completed=$boot"
  exit 1
fi

# 2. Package manager responding (system services up)
pm list packages -s >/dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "WAIT package_manager"
  exit 1
fi

# 3. Settings provider responding (UI layer ready)
settings get global window_animation_scale >/dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "WAIT settings_provider"
  exit 1
fi

echo "READY"
exit 0
`;

const READY_SCRIPT_PATH = '/data/local/tmp/jig-ready.sh';

/**
 * Push the readiness script to the device. Idempotent — overwrites if exists.
 */
async function pushReadyScript(serial: string): Promise<void> {
  // Write script content via stdin to avoid temp files on the host
  await execFileAsync('adb', [
    '-s', serial, 'shell',
    `cat > ${READY_SCRIPT_PATH} << 'JIGEOF'\n${READY_SCRIPT}JIGEOF`,
  ]);
  await execFileAsync('adb', ['-s', serial, 'shell', 'chmod', '755', READY_SCRIPT_PATH]);
  debug(`Pushed readiness script to ${READY_SCRIPT_PATH}`);
}

async function waitForBoot(serial: string, timeout: number): Promise<void> {
  const deadline = Date.now() + timeout;
  const startTime = Date.now();

  // Stage 1: wait-for-device (TCP connection)
  debug('Stage 1: adb wait-for-device');
  await execFileAsync('adb', ['-s', serial, 'wait-for-device']);

  // Stage 2: push readiness script and poll it
  debug('Stage 2: pushing readiness script');
  let scriptPushed = false;
  let lastStatus = '';

  while (Date.now() < deadline) {
    // Push script once the shell is responsive
    if (!scriptPushed) {
      try {
        await pushReadyScript(serial);
        scriptPushed = true;
      } catch {
        // Shell not ready yet — retry
        await sleep(2000);
        continue;
      }
    }

    // Poll the script — use shell exit code 0/1, capture stdout either way
    try {
      // Run with `; echo EXIT:$?` so we always get stdout even on non-zero exit
      const { stdout } = await execFileAsync('adb', [
        '-s', serial, 'shell', `sh ${READY_SCRIPT_PATH}; echo EXIT:$?`,
      ], { timeout: 10_000 });

      const lines = stdout.trim().split('\n');
      const statusLine = lines.find(l => !l.startsWith('EXIT:')) || '';

      if (statusLine === 'READY') {
        const elapsed = ((Date.now() - startTime) / 1000).toFixed(0);
        info(`Booted in ${elapsed}s`);
        return;
      }

      // Log status changes (e.g. "WAIT boot_completed=", "WAIT package_manager")
      if (statusLine && statusLine !== lastStatus) {
        debug(`Readiness: ${statusLine}`);
        lastStatus = statusLine;
      }
    } catch {
      // Shell disconnected or timeout
    }

    await sleep(1000);
  }

  throw new Error(
    `Emulator did not finish booting within ${timeout / 1000}s.\n` +
    `Last readiness status: ${lastStatus || 'unknown'}\n` +
    `This usually means the system image doesn't support the host's hardware acceleration.\n` +
    `Host architecture: ${process.arch}`,
  );
}

async function disableAnimations(serial: string): Promise<void> {
  debug('Disabling animations');
  const scales = [
    'window_animation_scale',
    'transition_animation_scale',
    'animator_duration_scale',
  ];
  for (const scale of scales) {
    try {
      await execFileAsync('adb', [
        '-s', serial, 'shell', 'settings', 'put', 'global', scale, '0.0',
      ]);
    } catch {
      // Best effort
    }
  }
}
