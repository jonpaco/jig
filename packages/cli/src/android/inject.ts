/**
 * APK injection pipeline for Jig.
 *
 * Required external tools:
 * - aapt2 (or aapt fallback): APK manifest inspection (Android SDK build-tools)
 * - zipalign: APK alignment (Android SDK build-tools)
 * - apksigner: APK signing (Android SDK build-tools)
 * - keytool: Java keystore management (JDK)
 * - java: JVM for running the dex-patcher (JDK)
 */

import { execFile } from 'child_process';
import { promisify } from 'util';
import fs from 'fs';
import path from 'path';
import os from 'os';
import { listEntries, addEntries, extractEntry, replaceEntry } from './zip';

const execFileAsync = promisify(execFile);

export interface InjectOptions {
  apkPath: string;
  libjigDir: string;        // directory containing per-ABI libjig.so (e.g., build/android/)
  helpersDexPath: string;   // path to jig-helpers.dex
  dexPatcherJarPath: string; // path to jig-dex-patcher.jar
  outputPath?: string;       // defaults to <apkName>-jig.apk next to original
}

/**
 * Inject libjig.so and jig-helpers.dex into an APK and re-sign it.
 *
 * Pipeline:
 *   1. Parse manifest via aapt2 (fallback aapt) to find launcher activity and package name
 *   2. Analyze existing APK ZIP entries to detect ABIs and count DEX files
 *   3. Add libjig.so to lib/<abi>/ for each ABI present (or arm64-v8a default)
 *   4. Add jig-helpers.dex as classesN+1.dex
 *   5. Find which classesX.dex contains the launcher activity (via dex-patcher exit codes)
 *   6. Extract that DEX, patch it via dex-patcher, replace it
 *   7. zipalign + apksigner with debug keystore (v1+v2 signing)
 *
 * Returns the path to the signed, aligned APK.
 */
export async function injectApk(options: InjectOptions): Promise<string> {
  const { apkPath, libjigDir, helpersDexPath, dexPatcherJarPath } = options;

  if (!fs.existsSync(apkPath)) {
    throw new Error(`APK not found at ${apkPath}`);
  }
  if (!fs.existsSync(libjigDir)) {
    throw new Error(`libjig directory not found at ${libjigDir}`);
  }
  if (!fs.existsSync(helpersDexPath)) {
    throw new Error(`jig-helpers.dex not found at ${helpersDexPath}`);
  }
  if (!fs.existsSync(dexPatcherJarPath)) {
    throw new Error(`jig-dex-patcher.jar not found at ${dexPatcherJarPath}`);
  }

  const apkBasename = path.basename(apkPath, '.apk');
  const outputPath = options.outputPath ??
    path.join(path.dirname(apkPath), `${apkBasename}-jig.apk`);

  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'jig-inject-'));

  try {
    // 1. Parse manifest to find launcher activity
    const { activityName } = await getPackageAndActivity(apkPath);

    // 2. Analyze existing ZIP entries for ABIs and DEX count
    const entries = await listEntries(apkPath);

    const abiSet = new Set<string>();
    for (const entry of entries) {
      const match = entry.match(/^lib\/([^/]+)\//);
      if (match) {
        abiSet.add(match[1]);
      }
    }
    const targetAbis = abiSet.size > 0 ? [...abiSet] : ['arm64-v8a'];

    const dexEntries = entries
      .filter((e) => /^classes\d*\.dex$/.test(e))
      .sort();
    const dexCount = dexEntries.length;

    // 3. Build new entries: libjig.so for each ABI
    const newEntries: Record<string, Buffer> = {};

    const builtAbis = fs.readdirSync(libjigDir).filter((name) => {
      const soPath = path.join(libjigDir, name, 'libjig.so');
      return fs.existsSync(soPath);
    });
    if (builtAbis.length === 0) {
      throw new Error(`No libjig.so files found in ${libjigDir}`);
    }

    for (const abi of targetAbis) {
      const srcSo = path.join(libjigDir, abi, 'libjig.so');
      if (!fs.existsSync(srcSo)) {
        console.warn(`Warning: no libjig.so built for ${abi}, skipping`);
        continue;
      }
      newEntries[`lib/${abi}/libjig.so`] = fs.readFileSync(srcSo);
    }

    // 4. Add jig-helpers.dex as classesN+1.dex
    const helpersDexName = `classes${dexCount + 1}.dex`;
    newEntries[helpersDexName] = fs.readFileSync(helpersDexPath);

    // Add new entries to the APK
    const withEntriesApk = path.join(tmpDir, 'with-entries.apk');
    await addEntries(apkPath, withEntriesApk, newEntries);

    // 5. Find which DEX contains the launcher activity and patch it
    const dexToPath = await findDexContainingClass(
      withEntriesApk,
      dexEntries,
      activityName,
      dexPatcherJarPath,
      tmpDir,
    );

    // 6. Extract that DEX, patch it, replace it
    const extractedDex = path.join(tmpDir, 'target.dex');
    const patchedDex = path.join(tmpDir, 'patched.dex');

    const dexData = await extractEntry(withEntriesApk, dexToPath);
    fs.writeFileSync(extractedDex, dexData);

    await execFileAsync('java', [
      '-jar', dexPatcherJarPath,
      extractedDex,
      activityName,
      patchedDex,
    ]);

    const patchedApk = path.join(tmpDir, 'patched.apk');
    await replaceEntry(
      withEntriesApk,
      patchedApk,
      dexToPath,
      fs.readFileSync(patchedDex),
    );

    // 7. zipalign + apksigner
    const alignedApk = path.join(tmpDir, 'aligned.apk');
    await execFileAsync('zipalign', ['-f', '4', patchedApk, alignedApk]);

    const keystore = await findOrCreateDebugKeystore();
    await execFileAsync('apksigner', [
      'sign',
      '--ks', keystore,
      '--ks-pass', 'pass:android',
      '--ks-key-alias', 'androiddebugkey',
      '--key-pass', 'pass:android',
      '--v1-signing-enabled', 'true',
      '--v2-signing-enabled', 'true',
      '--out', outputPath,
      alignedApk,
    ]);

    return outputPath;
  } finally {
    fs.rmSync(tmpDir, { recursive: true, force: true });
  }
}

/**
 * Extract package name and launcher activity from an APK using aapt2 (with aapt fallback).
 */
export async function getPackageAndActivity(apkPath: string): Promise<{
  packageName: string;
  activityName: string;
}> {
  let stdout: string;

  try {
    const result = await execFileAsync('aapt2', ['dump', 'badging', apkPath]);
    stdout = result.stdout;
  } catch {
    // Fallback to aapt
    const result = await execFileAsync('aapt', ['dump', 'badging', apkPath]);
    stdout = result.stdout;
  }

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

/**
 * Find which DEX file in the APK contains the given class.
 *
 * Tries the dex-patcher on each classesN.dex. Exit code 2 from the patcher
 * means "class not found" — try the next DEX. Any other non-zero exit is
 * a real error and is thrown. When a DEX is patched successfully (exit 0),
 * the dummy output is cleaned up and the entry name is returned.
 */
async function findDexContainingClass(
  apkPath: string,
  dexEntries: string[],
  className: string,
  dexPatcherJarPath: string,
  tmpDir: string,
): Promise<string> {
  for (const dexEntry of dexEntries) {
    const probeDex = path.join(tmpDir, `probe-${dexEntry}`);
    const probeOut = path.join(tmpDir, `probe-out-${dexEntry}`);

    const dexData = await extractEntry(apkPath, dexEntry);
    fs.writeFileSync(probeDex, dexData);

    try {
      await execFileAsync('java', [
        '-jar', dexPatcherJarPath,
        probeDex,
        className,
        probeOut,
      ]);
      // Success — this DEX contains the class. Clean up the dummy output.
      try { fs.unlinkSync(probeOut); } catch { /* ignore */ }
      try { fs.unlinkSync(probeDex); } catch { /* ignore */ }
      return dexEntry;
    } catch (err: unknown) {
      // Exit code 2 means class not found — try next DEX
      const exitCode = (err as { code?: number }).code;
      if (exitCode === 2) {
        try { fs.unlinkSync(probeDex); } catch { /* ignore */ }
        continue;
      }
      // Any other error is unexpected
      throw err;
    }
  }

  throw new Error(
    `Could not find class ${className} in any DEX file. ` +
    `Searched: ${dexEntries.join(', ')}`,
  );
}

/**
 * Find the Android debug keystore, or create one if it doesn't exist.
 * Standard location: ~/.android/debug.keystore
 */
export async function findOrCreateDebugKeystore(): Promise<string> {
  const debugKeystore = path.join(os.homedir(), '.android', 'debug.keystore');

  if (fs.existsSync(debugKeystore)) {
    return debugKeystore;
  }

  // Create the directory if needed
  const dir = path.dirname(debugKeystore);
  fs.mkdirSync(dir, { recursive: true });

  // Create a debug keystore matching Android Studio's defaults
  await execFileAsync('keytool', [
    '-genkeypair',
    '-alias', 'androiddebugkey',
    '-keypass', 'android',
    '-keystore', debugKeystore,
    '-storepass', 'android',
    '-dname', 'CN=Android Debug,O=Android,C=US',
    '-keyalg', 'RSA',
    '-keysize', '2048',
    '-validity', '10000',
  ]);

  return debugKeystore;
}
