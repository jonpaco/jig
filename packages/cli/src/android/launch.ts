// Required external tools: adb (Android SDK platform-tools),
// plus all tools listed in inject.ts (aapt2, zipalign, apksigner, keytool, java).

import path from 'path';
import { injectApk, getPackageAndActivity } from './inject';
import { resolveLibjigDir, resolveHelpersDex, resolveDexPatcher } from './resolve';
import { ensureDevice } from './emulator';
import { execFileAsync } from '@jig/device';
import fs from 'fs';

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

  // Ensure a device/emulator is available
  const device = await ensureDevice();
  if (!device.alreadyRunning) {
    process.stderr.write('No device connected. Started Jig emulator.\n');
  }

  // Resolve libjig.so directory
  const libjigDir = resolveLibjigDir(libjig);

  // Inject into APK
  const helpersDexPath = resolveHelpersDex();
  const dexPatcherJarPath = resolveDexPatcher();

  const patchedApk = await injectApk({
    apkPath,
    libjigDir,
    helpersDexPath,
    dexPatcherJarPath,
  });

  // Install via adb
  await execFileAsync('adb', ['-s', device.serial, 'install', '-r', '-t', patchedApk]);

  // Push fiber walker JS to device for JS bridge injection
  const cliRoot = path.resolve(__dirname, '..', '..');
  const fiberWalkerDistPath = path.join(cliRoot, 'assets', 'fiber-walker.js');
  const fiberWalkerDevPath = path.resolve(cliRoot, '..', 'native', 'core', 'js', 'fiber-walker.js');
  const fiberWalkerPath = fs.existsSync(fiberWalkerDistPath) ? fiberWalkerDistPath : fiberWalkerDevPath;
  if (fs.existsSync(fiberWalkerPath)) {
    await execFileAsync('adb', ['-s', device.serial, 'push', fiberWalkerPath, '/data/local/tmp/jig-fiber-walker.js']);
  }

  // Parse the original APK's manifest to find the main activity for launch.
  // We re-read from the patched APK's decoded manifest isn't available after
  // cleanup, so we use aapt to extract package/activity from the patched APK.
  const { packageName, activityName } = await getPackageAndActivity(patchedApk);

  // Forward the Jig port if needed
  await execFileAsync('adb', [
    '-s', device.serial, 'forward', `tcp:${port}`, `tcp:${port}`,
  ]);

  // Launch the activity
  const component = `${packageName}/${activityName}`;
  await execFileAsync('adb', [
    '-s', device.serial, 'shell', 'am', 'start', '-n', component,
  ]);

  // Clean up patched APK
  try { fs.unlinkSync(patchedApk); } catch { /* ignore */ }

  return (
    `Installed and launched ${packageName} on device\n` +
    `Jig server starting on port ${port}...`
  );
}

