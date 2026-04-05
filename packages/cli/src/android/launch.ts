// Required external tools: adb (Android SDK platform-tools),
// plus all tools listed in inject.ts (apktool, zipalign, apksigner, aapt, keytool).

import { execFile } from 'child_process';
import { promisify } from 'util';
import path from 'path';
import { injectApk, findMainActivity } from './inject';
import { resolveLibjigDir } from './resolve';
import fs from 'fs';

const execFileAsync = promisify(execFile);

export interface AndroidLaunchOptions {
  apkPath: string;
  libjig?: string;  // explicit path to libjig dir
  port?: number;
}

/**
 * Inject libjig.so into an APK, install it on a connected device/emulator,
 * and launch the main activity.
 */
export async function launchAndroid(options: AndroidLaunchOptions): Promise<string> {
  const { apkPath, libjig, port = 4042 } = options;

  // Resolve libjig.so directory
  const libjigDir = resolveLibjigDir(libjig);

  // Inject into APK
  const patchedApk = await injectApk({
    apkPath,
    libjigDir,
  });

  // Install via adb
  await execFileAsync('adb', ['install', '-r', '-t', patchedApk]);

  // Parse the original APK's manifest to find the main activity for launch.
  // We re-read from the patched APK's decoded manifest isn't available after
  // cleanup, so we use aapt to extract package/activity from the patched APK.
  const { packageName, activityName } = await getPackageAndActivity(patchedApk);

  // Forward the Jig port if needed
  await execFileAsync('adb', [
    'forward', `tcp:${port}`, `tcp:${port}`,
  ]);

  // Launch the activity
  const component = `${packageName}/${activityName}`;
  await execFileAsync('adb', [
    'shell', 'am', 'start', '-n', component,
  ]);

  // Clean up patched APK
  try { fs.unlinkSync(patchedApk); } catch { /* ignore */ }

  return (
    `Installed and launched ${packageName} on device\n` +
    `Jig server starting on port ${port}...`
  );
}

/**
 * Extract package name and main activity from an APK using aapt.
 */
async function getPackageAndActivity(apkPath: string): Promise<{
  packageName: string;
  activityName: string;
}> {
  const { stdout } = await execFileAsync('aapt', [
    'dump', 'badging', apkPath,
  ]);

  const packageMatch = stdout.match(/package:\s*name='([^']+)'/);
  if (!packageMatch) {
    throw new Error('Could not extract package name from APK');
  }

  const activityMatch = stdout.match(/launchable-activity:\s*name='([^']+)'/);
  if (!activityMatch) {
    throw new Error('Could not extract launchable activity from APK');
  }

  return {
    packageName: packageMatch[1],
    activityName: activityMatch[1],
  };
}
