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
export function resolveSDKTool(tool: string): string {
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
  return path.join(androidHome, subdir, tool);
}

/**
 * Find the best installed system image for the given ABI.
 * Prefers highest API level, then google_apis > default > google_apis_playstore.
 * Returns null if no matching image found.
 */
export async function findBestSystemImage(abi: string): Promise<string | null> {
  const sdkmanager = resolveSDKTool('sdkmanager');

  let stdout: string;
  try {
    const result = await execFileAsync(sdkmanager, ['--list_installed']);
    stdout = result.stdout;
  } catch {
    try {
      const result = await execFileAsync(sdkmanager, ['--list']);
      stdout = result.stdout;
    } catch {
      return null;
    }
  }

  const imagePattern = /system-images;android-(\d+);([^;\s|]+);([^;\s|]+)/g;
  const images: Array<{ full: string; api: number; target: string; arch: string }> = [];

  let match;
  while ((match = imagePattern.exec(stdout)) !== null) {
    images.push({
      full: match[0],
      api: parseInt(match[1], 10),
      target: match[2],
      arch: match[3],
    });
  }

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
    const avdmanager = resolveSDKTool('avdmanager');
    const { stdout } = await execFileAsync(avdmanager, ['list', 'avd', '-c']);
    const avds = stdout.trim().split('\n').map((l) => l.trim()).filter(Boolean);
    if (!avds.includes(JIG_AVD_NAME)) return null;

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
  const avdmanager = resolveSDKTool('avdmanager');
  await execFileAsync(avdmanager, ['delete', 'avd', '-n', JIG_AVD_NAME]);
}

async function createAVD(systemImage: string): Promise<void> {
  const avdmanager = resolveSDKTool('avdmanager');
  await execFileAsync(avdmanager, [
    'create', 'avd',
    '-n', JIG_AVD_NAME,
    '-k', systemImage,
    '-d', 'pixel_6',
    '--force',
  ]);
}

async function bootEmulator(): Promise<string> {
  const emulatorBin = resolveSDKTool('emulator');
  const { spawn } = require('child_process');

  const proc = spawn(emulatorBin, [
    '-avd', JIG_AVD_NAME,
    '-no-window',
    '-no-audio',
    '-no-boot-anim',
    '-no-snapshot-save',
    '-gpu', 'swiftshader_indirect',
  ], {
    detached: true,
    stdio: 'ignore',
  });

  proc.unref();

  const deadline = Date.now() + 30_000;
  while (Date.now() < deadline) {
    const devices = await getConnectedDevices();
    const emulator = devices.find((d) => d.startsWith('emulator-'));
    if (emulator) return emulator;
    await sleep(1000);
  }

  throw new Error(
    `Emulator started but not detected by adb within 30s.\n` +
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
