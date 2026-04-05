/**
 * Android emulator lifecycle management for `jig launch`.
 *
 * When no device/emulator is connected, Jig creates and boots one
 * with the right configuration for the host architecture. In CI
 * (detected via environment variables), the emulator is killed on
 * process exit. Locally it stays running for fast iteration.
 */

import { execFile } from 'child_process';
import { promisify } from 'util';
import path from 'path';
import fs from 'fs';

const execFileAsync = promisify(execFile);

export const JIG_AVD_NAME = 'jig-emulator';

/**
 * List connected devices/emulators that are in "device" state.
 * Returns empty array if adb is unavailable or no devices connected.
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
 * Map Node.js process.arch to Android ABI.
 */
export function getHostArch(arch: string = process.arch): string {
  switch (arch) {
    case 'arm64':
      return 'arm64-v8a';
    case 'x64':
      return 'x86_64';
    default:
      throw new Error(
        `Unsupported host architecture: ${arch}. ` +
        `Jig supports arm64 (Apple Silicon / ARM Linux) and x64 (Intel).`,
      );
  }
}

/**
 * Resolve the path to an Android SDK tool.
 */
export function resolveSDKTool(tool: string, mustExist: boolean = false): string {
  const androidHome = process.env.ANDROID_HOME || process.env.ANDROID_SDK_ROOT;
  if (!androidHome) {
    throw new Error(
      `ANDROID_HOME not set and Android SDK not found.\n` +
      `Install Android SDK: https://developer.android.com/studio/command-line`,
    );
  }

  const subdirs: Record<string, string> = {
    adb: 'platform-tools',
    emulator: 'emulator',
    avdmanager: path.join('cmdline-tools', 'latest', 'bin'),
    sdkmanager: path.join('cmdline-tools', 'latest', 'bin'),
  };

  const subdir = subdirs[tool] || 'tools/bin';
  const toolPath = path.join(androidHome, subdir, tool);

  if (mustExist && !fs.existsSync(toolPath)) {
    const cmdlineToolsNeeded = ['avdmanager', 'sdkmanager'].includes(tool);
    const installHint = cmdlineToolsNeeded
      ? `Install Android command-line tools:\n` +
        `  Android Studio → Settings → SDK Manager → SDK Tools → Android SDK Command-line Tools\n` +
        `  Or download from: https://developer.android.com/studio#command-line-tools-only`
      : `Expected at: ${toolPath}`;
    throw new Error(`Android ${tool} not found.\n${installHint}`);
  }

  return toolPath;
}

/**
 * Find the best installed system image for the given ABI.
 * Prefers highest API level, then google_apis > default > google_apis_playstore.
 * Returns null if no matching image found.
 */
export async function findBestSystemImage(abi: string): Promise<string | null> {
  // Try sdkmanager first
  const images = await findImagesViaSdkmanager() || await findImagesViaFilesystem();
  if (!images || images.length === 0) return null;

  const matching = images.filter((img) => img.arch === abi);
  if (matching.length === 0) return null;

  const targetPriority: Record<string, number> = {
    google_apis: 0,
    default: 1,
    google_apis_playstore: 2,
  };

  matching.sort((a, b) => {
    if (b.api !== a.api) return b.api - a.api;
    return (targetPriority[a.target] ?? 99) - (targetPriority[b.target] ?? 99);
  });

  return matching[0].full;
}

interface SystemImage {
  full: string;
  api: number;
  target: string;
  arch: string;
}

function parseImageString(stdout: string): SystemImage[] {
  const imagePattern = /system-images;android-([\d.]+);([^;\s|]+);([^;\s|]+)/g;
  const images: SystemImage[] = [];

  let match;
  while ((match = imagePattern.exec(stdout)) !== null) {
    images.push({
      full: match[0],
      api: parseFloat(match[1]),
      target: match[2],
      arch: match[3],
    });
  }
  return images;
}

async function findImagesViaSdkmanager(): Promise<SystemImage[] | null> {
  try {
    const sdkmanager = resolveSDKTool('sdkmanager');
    let stdout: string;
    try {
      const result = await execFileAsync(sdkmanager, ['--list_installed']);
      stdout = result.stdout;
    } catch {
      const result = await execFileAsync(sdkmanager, ['--list']);
      stdout = result.stdout;
    }
    const images = parseImageString(stdout);
    return images.length > 0 ? images : null;
  } catch {
    return null;
  }
}

async function findImagesViaFilesystem(): Promise<SystemImage[] | null> {
  const androidHome = process.env.ANDROID_HOME || process.env.ANDROID_SDK_ROOT;
  if (!androidHome) return null;

  const imagesDir = path.join(androidHome, 'system-images');
  if (!fs.existsSync(imagesDir)) return null;

  const images: SystemImage[] = [];

  try {
    // Structure: system-images/android-{N}/{target}/{abi}/
    for (const apiDir of fs.readdirSync(imagesDir)) {
      const apiMatch = apiDir.match(/^android-([\d.]+)$/);
      if (!apiMatch) continue;

      const api = parseFloat(apiMatch[1]);
      const targetDir = path.join(imagesDir, apiDir);

      for (const target of fs.readdirSync(targetDir)) {
        const abiDir = path.join(targetDir, target);
        if (!fs.statSync(abiDir).isDirectory()) continue;

        for (const abi of fs.readdirSync(abiDir)) {
          const fullPath = path.join(abiDir, abi);
          if (!fs.statSync(fullPath).isDirectory()) continue;

          images.push({
            full: `system-images;android-${apiMatch[1]};${target};${abi}`,
            api,
            target,
            arch: abi,
          });
        }
      }
    }
  } catch {
    return null;
  }

  return images.length > 0 ? images : null;
}

const isCI = !!(
  process.env.CI ||
  process.env.GITHUB_ACTIONS ||
  process.env.JENKINS_URL ||
  process.env.CIRCLECI
);

export interface EnsureDeviceResult {
  serial: string;
  alreadyRunning: boolean;
  booted: boolean;
}

/**
 * Ensure an Android device or emulator is available.
 * If none connected, creates and boots a Jig-managed emulator.
 */
export async function ensureDevice(): Promise<EnsureDeviceResult> {
  const devices = await getConnectedDevices();
  if (devices.length > 0) {
    return { serial: devices[0], alreadyRunning: true, booted: false };
  }

  const abi = getHostArch();
  const image = await findBestSystemImage(abi);

  if (!image) {
    throw new Error(
      `No Android system image found for ${abi}.\n` +
      `Install one with:\n` +
      `  sdkmanager "system-images;android-34;google_apis;${abi}"`,
    );
  }

  const existingAVD = await getExistingAVD();
  if (existingAVD && existingAVD.arch !== abi) {
    await deleteAVD();
  }

  if (!existingAVD || existingAVD.arch !== abi) {
    await createAVD(image);
  }

  const serial = await bootEmulator();
  await waitForBoot(serial);
  await disableAnimations(serial);

  if (isCI) {
    registerCleanup(serial);
  }

  return { serial, alreadyRunning: false, booted: true };
}

interface AVDInfo {
  name: string;
  arch: string;
}

async function getExistingAVD(): Promise<AVDInfo | null> {
  try {
    const avdHome = process.env.ANDROID_AVD_HOME ||
      path.join(process.env.HOME || '', '.android', 'avd');
    const configPath = path.join(avdHome, `${JIG_AVD_NAME}.avd`, 'config.ini');

    if (!fs.existsSync(configPath)) return null;

    const config = fs.readFileSync(configPath, 'utf-8');
    const abiMatch = config.match(/abi\.type=(.+)/);
    const arch = abiMatch ? abiMatch[1].trim() : 'unknown';

    return { name: JIG_AVD_NAME, arch };
  } catch {
    return null;
  }
}

async function deleteAVD(): Promise<void> {
  const avdHome = process.env.ANDROID_AVD_HOME ||
    path.join(process.env.HOME || '', '.android', 'avd');
  const avdDir = path.join(avdHome, `${JIG_AVD_NAME}.avd`);
  const iniFile = path.join(avdHome, `${JIG_AVD_NAME}.ini`);

  if (fs.existsSync(avdDir)) fs.rmSync(avdDir, { recursive: true });
  if (fs.existsSync(iniFile)) fs.unlinkSync(iniFile);
}

async function createAVD(systemImage: string): Promise<void> {
  // Write AVD files directly instead of using avdmanager.
  // avdmanager creates bloated configs with broken defaults (hw.gpu.enabled=no,
  // 256MB RAM, placeholder values) that fail to boot headless on Apple Silicon.
  const avdHome = process.env.ANDROID_AVD_HOME ||
    path.join(process.env.HOME || '', '.android', 'avd');
  const androidHome = process.env.ANDROID_HOME || process.env.ANDROID_SDK_ROOT || '';

  const avdDir = path.join(avdHome, `${JIG_AVD_NAME}.avd`);
  fs.mkdirSync(avdDir, { recursive: true });

  // Parse the system image string: system-images;android-{api};{target};{abi}
  const parts = systemImage.split(';');
  const apiLevel = parts[1]?.replace('android-', '') || '34';
  const target = parts[2] || 'google_apis';
  const abi = parts[3] || 'arm64-v8a';
  const imagePath = `system-images/${parts[1]}/${target}/${abi}/`;
  const isPlayStore = target === 'google_apis_playstore';

  // Write config.ini — minimal working config modeled on Android Studio output
  const config = [
    `AvdId=${JIG_AVD_NAME}`,
    `PlayStore.enabled=${isPlayStore}`,
    `abi.type=${abi}`,
    `avd.ini.displayname=Jig Emulator`,
    `avd.ini.encoding=UTF-8`,
    `disk.dataPartition.size=6G`,
    `fastboot.chosenSnapshotFile=`,
    `fastboot.forceChosenSnapshotBoot=no`,
    `fastboot.forceColdBoot=no`,
    `fastboot.forceFastBoot=yes`,
    `hw.accelerometer=yes`,
    `hw.arc=false`,
    `hw.audioInput=yes`,
    `hw.battery=yes`,
    `hw.camera.back=none`,
    `hw.camera.front=none`,
    `hw.cpu.arch=${abi === 'x86_64' ? 'x86_64' : 'arm64'}`,
    `hw.cpu.ncore=4`,
    `hw.dPad=no`,
    `hw.device.manufacturer=Google`,
    `hw.device.name=pixel_6`,
    `hw.gps=yes`,
    `hw.gpu.enabled=yes`,
    `hw.gpu.mode=auto`,
    `hw.keyboard=yes`,
    `hw.lcd.density=420`,
    `hw.lcd.height=2400`,
    `hw.lcd.width=1080`,
    `hw.mainKeys=no`,
    `hw.ramSize=4096`,
    `hw.sdCard=no`,
    `hw.sensors.orientation=yes`,
    `hw.sensors.proximity=yes`,
    `hw.trackBall=no`,
    `image.sysdir.1=${imagePath}`,
    `runtime.network.latency=none`,
    `runtime.network.speed=full`,
    `showDeviceFrame=no`,
    `skin.dynamic=yes`,
    `skin.name=1080x2400`,
    `skin.path=1080x2400`,
    `tag.display=${isPlayStore ? 'Google Play' : 'Google APIs'}`,
    `tag.id=${target}`,
    `target=android-${apiLevel}`,
    `vm.heapSize=256`,
  ].join('\n') + '\n';

  fs.writeFileSync(path.join(avdDir, 'config.ini'), config);

  // Write the .ini pointer file that tells the emulator where the AVD lives
  const iniContent = [
    `avd.ini.encoding=UTF-8`,
    `path=${avdDir}`,
    `path.rel=avd/${JIG_AVD_NAME}.avd`,
    `target=android-${apiLevel}`,
  ].join('\n') + '\n';

  fs.writeFileSync(path.join(avdHome, `${JIG_AVD_NAME}.ini`), iniContent);
}

async function bootEmulator(): Promise<string> {
  const emulatorBin = resolveSDKTool('emulator', true);
  const { spawn } = require('child_process');

  // Notes on emulator flags:
  // - No -no-window: the headless binary lacks com.apple.security.hypervisor
  //   entitlement on macOS, disabling HVF and making boot 10x slower.
  // - No -no-snapshot-save: snapshots make subsequent boots instant (~1s vs 60s+).
  //   First cold boot is slow but saves a snapshot for future runs.
  // - gpu=auto: uses host GPU (Metal on macOS) when available, falls back to software.
  const proc = spawn(emulatorBin, [
    '-avd', JIG_AVD_NAME,
    '-no-audio',
    '-no-boot-anim',
    '-gpu', 'auto',
  ], {
    detached: true,
    stdio: 'ignore',
  });

  proc.unref();

  // Wait for adb to see the emulator (in any state — including offline).
  // CI cold boots can take 60s+ before adb even sees the device.
  const deadline = Date.now() + 120_000;
  while (Date.now() < deadline) {
    try {
      const { stdout } = await execFileAsync('adb', ['devices']);
      const lines = stdout.split('\n').slice(1);
      for (const line of lines) {
        const parts = line.trim().split('\t');
        if (parts[0]?.startsWith('emulator-') && parts[1]) {
          return parts[0];
        }
      }
    } catch {
      // adb not ready yet
    }
    await sleep(2000);
  }

  throw new Error(
    `Emulator started but not detected by adb within 120s.\n` +
    `Check that the Android emulator is installed:\n` +
    `  sdkmanager "emulator"`,
  );
}

async function waitForBoot(serial: string, timeoutMs: number = 180_000): Promise<void> {
  const deadline = Date.now() + timeoutMs;
  const startTime = Date.now();

  await execFileAsync('adb', ['-s', serial, 'wait-for-device']);

  while (Date.now() < deadline) {
    try {
      const { stdout } = await execFileAsync('adb', [
        '-s', serial, 'shell', 'getprop', 'sys.boot_completed',
      ]);
      if (stdout.trim() === '1') break;
    } catch {
      // Device not ready yet
    }
    await sleep(2000);
  }

  if (Date.now() >= deadline) {
    throw new Error(
      `Emulator did not finish booting within ${timeoutMs / 1000}s.\n` +
      `This usually means the system image doesn't support the host's hardware acceleration.\n` +
      `Host architecture: ${process.arch}`,
    );
  }

  const animDeadline = Date.now() + 30_000;
  while (Date.now() < animDeadline) {
    try {
      const { stdout } = await execFileAsync('adb', [
        '-s', serial, 'shell', 'getprop', 'init.svc.bootanim',
      ]);
      if (stdout.trim() === 'stopped') break;
    } catch {
      // Not ready yet
    }
    await sleep(1000);
  }

  const elapsed = ((Date.now() - startTime) / 1000).toFixed(0);
  process.stderr.write(`  Booted in ${elapsed}s\n`);
}

async function disableAnimations(serial: string): Promise<void> {
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

function registerCleanup(serial: string): void {
  const cleanup = () => {
    try {
      const { execFileSync } = require('child_process');
      execFileSync('adb', ['-s', serial, 'emu', 'kill'], { timeout: 5000 });
    } catch {
      // Best effort
    }
  };

  process.on('exit', cleanup);
  process.on('SIGINT', () => { cleanup(); process.exit(130); });
  process.on('SIGTERM', () => { cleanup(); process.exit(143); });
}

function sleep(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms));
}
