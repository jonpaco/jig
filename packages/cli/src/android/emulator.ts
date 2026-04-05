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
