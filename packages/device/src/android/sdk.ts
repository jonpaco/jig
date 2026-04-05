import { execFile } from 'child_process';
import { promisify } from 'util';
import path from 'path';
import fs from 'fs';

const execFileAsync = promisify(execFile);

/**
 * Map Node.js process.arch to Android ABI.
 */
export function getHostAbi(arch: string = process.arch): string {
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
 * Resolve an Android ABI from a profile's arch field.
 * 'auto' detects from host CPU.
 */
export function resolveAbi(arch: 'auto' | 'x86_64' | 'arm64-v8a'): string {
  return arch === 'auto' ? getHostAbi() : arch;
}

const SDK_TOOL_SUBDIRS: Record<string, string> = {
  adb: 'platform-tools',
  emulator: 'emulator',
  avdmanager: path.join('cmdline-tools', 'latest', 'bin'),
  sdkmanager: path.join('cmdline-tools', 'latest', 'bin'),
};

/**
 * Resolve the path to an Android SDK tool.
 */
export function resolveSDKTool(tool: string, mustExist: boolean = false): string {
  const androidHome = process.env.ANDROID_HOME || process.env.ANDROID_SDK_ROOT;
  if (!androidHome) {
    throw new Error(
      `ANDROID_HOME not set. Install Android Studio or set ANDROID_HOME to your SDK path:\n` +
      `  https://developer.android.com/studio`,
    );
  }

  const subdir = SDK_TOOL_SUBDIRS[tool] || 'tools/bin';
  const toolPath = path.join(androidHome, subdir, tool);

  if (mustExist && !fs.existsSync(toolPath)) {
    const cmdlineToolsNeeded = ['avdmanager', 'sdkmanager'].includes(tool);
    const installHint = cmdlineToolsNeeded
      ? `Install Android SDK Command-line Tools via Android Studio > SDK Manager > SDK Tools tab`
      : `Expected at: ${toolPath}`;
    throw new Error(`Android ${tool} not found.\n${installHint}`);
  }

  return toolPath;
}

export interface SystemImage {
  full: string;
  api: number;
  target: string;
  arch: string;
}

export function parseImageString(stdout: string): SystemImage[] {
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

/**
 * Find the best installed system image for the given ABI.
 * Prefers highest API level, then google_apis > default > google_apis_playstore.
 */
export async function findBestSystemImage(abi: string): Promise<string | null> {
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

/**
 * Find a specific system image matching api, target, and abi.
 */
export async function findSystemImage(
  api: number,
  target: string,
  abi: string,
): Promise<string | null> {
  const images = await findImagesViaSdkmanager() || await findImagesViaFilesystem();
  if (!images) return null;

  const match = images.find(
    (img) => img.api === api && img.target === target && img.arch === abi,
  );
  return match?.full ?? null;
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
