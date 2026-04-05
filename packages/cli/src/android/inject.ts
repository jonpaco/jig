import { execFile } from 'child_process';
import { promisify } from 'util';
import fs from 'fs';
import path from 'path';
import os from 'os';

const execFileAsync = promisify(execFile);

export interface InjectOptions {
  apkPath: string;
  libjigDir: string;   // directory containing per-ABI libjig.so (e.g., build/android/)
  outputPath?: string;  // defaults to <apkName>-jig.apk next to original
}

/**
 * Inject libjig.so into an APK and re-sign it.
 *
 * Pipeline:
 *   1. apktool decode → temp dir
 *   2. Find main (launcher) activity from AndroidManifest.xml
 *   3. Copy libjig.so into lib/<abi>/ for each ABI present
 *   4. Patch main activity's smali to call System.loadLibrary("jig")
 *   5. apktool rebuild → unsigned APK
 *   6. zipalign
 *   7. apksigner with debug keystore
 *
 * Returns the path to the signed, aligned APK.
 */
export async function injectApk(options: InjectOptions): Promise<string> {
  const { apkPath, libjigDir } = options;

  if (!fs.existsSync(apkPath)) {
    throw new Error(`APK not found at ${apkPath}`);
  }
  if (!fs.existsSync(libjigDir)) {
    throw new Error(`libjig directory not found at ${libjigDir}`);
  }

  const apkBasename = path.basename(apkPath, '.apk');
  const outputPath = options.outputPath ??
    path.join(path.dirname(apkPath), `${apkBasename}-jig.apk`);

  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'jig-inject-'));

  try {
    const decodedDir = path.join(tmpDir, 'decoded');
    const rebuiltApk = path.join(tmpDir, 'rebuilt.apk');
    const alignedApk = path.join(tmpDir, 'aligned.apk');

    // 1. Decode APK
    await execFileAsync('apktool', ['d', '-f', '-o', decodedDir, apkPath]);

    // 2. Find main activity
    const manifestPath = path.join(decodedDir, 'AndroidManifest.xml');
    const mainActivity = findMainActivity(
      fs.readFileSync(manifestPath, 'utf-8'),
    );

    // 3. Copy libjig.so into lib dirs
    copyLibjig(decodedDir, libjigDir);

    // 4. Patch smali
    const smaliPath = resolveSmaliPath(decodedDir, mainActivity);
    patchSmali(smaliPath);

    // 5. Rebuild APK
    await execFileAsync('apktool', ['b', '-o', rebuiltApk, decodedDir]);

    // 6. Zipalign
    await execFileAsync('zipalign', ['-f', '4', rebuiltApk, alignedApk]);

    // 7. Sign with debug key
    const keystore = await findOrCreateDebugKeystore();
    await execFileAsync('apksigner', [
      'sign',
      '--ks', keystore,
      '--ks-pass', 'pass:android',
      '--ks-key-alias', 'androiddebugkey',
      '--key-pass', 'pass:android',
      '--out', outputPath,
      alignedApk,
    ]);

    return outputPath;
  } finally {
    // Clean up temp dir
    fs.rmSync(tmpDir, { recursive: true, force: true });
  }
}

/**
 * Parse AndroidManifest.xml to find the launcher activity's fully-qualified
 * class name. Looks for an activity with MAIN + LAUNCHER intent filter.
 */
export function findMainActivity(manifestXml: string): string {
  // apktool decodes to plain XML, so we can regex-parse the intent filters.
  // We look for an <activity> block that contains both:
  //   action android.intent.action.MAIN
  //   category android.intent.category.LAUNCHER

  // Split on <activity to get individual activity blocks
  const activityBlocks = manifestXml.split(/<activity\b/);

  for (const block of activityBlocks) {
    // Check for launcher intent
    const hasMain = /android\.intent\.action\.MAIN/.test(block);
    const hasLauncher = /android\.intent\.category\.LAUNCHER/.test(block);

    if (hasMain && hasLauncher) {
      // Extract android:name from this activity tag
      const nameMatch = block.match(/android:name="([^"]+)"/);
      if (nameMatch) {
        return nameMatch[1];
      }
    }
  }

  throw new Error(
    'Could not find launcher activity in AndroidManifest.xml. ' +
    'Expected an activity with MAIN + LAUNCHER intent filter.',
  );
}

/**
 * Copy libjig.so into the decoded APK's lib directories.
 * For each ABI directory already present in the APK, copies the matching
 * libjig.so from libjigDir. If no lib/ directory exists, creates arm64-v8a.
 */
function copyLibjig(decodedDir: string, libjigDir: string): void {
  const libDir = path.join(decodedDir, 'lib');

  // Get list of ABIs we have built
  const builtAbis = fs.readdirSync(libjigDir).filter((name) => {
    const soPath = path.join(libjigDir, name, 'libjig.so');
    return fs.existsSync(soPath);
  });

  if (builtAbis.length === 0) {
    throw new Error(`No libjig.so files found in ${libjigDir}`);
  }

  // Determine which ABIs to inject into
  let targetAbis: string[];
  if (fs.existsSync(libDir)) {
    // Match existing ABIs in the APK
    targetAbis = fs.readdirSync(libDir).filter((name) =>
      fs.statSync(path.join(libDir, name)).isDirectory(),
    );
  } else {
    // No native libs yet — default to arm64-v8a
    targetAbis = ['arm64-v8a'];
  }

  for (const abi of targetAbis) {
    const srcSo = path.join(libjigDir, abi, 'libjig.so');
    if (!fs.existsSync(srcSo)) {
      console.warn(`Warning: no libjig.so built for ${abi}, skipping`);
      continue;
    }

    const dstDir = path.join(libDir, abi);
    fs.mkdirSync(dstDir, { recursive: true });
    fs.copyFileSync(srcSo, path.join(dstDir, 'libjig.so'));
  }
}

/**
 * Resolve the smali file path for a given activity class name.
 * Handles apktool's smali directory naming (smali, smali_classes2, etc.)
 * and dotted vs slashed class names.
 */
export function resolveSmaliPath(decodedDir: string, activityName: string): string {
  // Convert com.example.MainActivity to com/example/MainActivity.smali
  const relativePath = activityName.replace(/\./g, '/') + '.smali';

  // apktool can put smali in smali/, smali_classes2/, etc.
  const smaliDirs = fs.readdirSync(decodedDir).filter((name) =>
    name.startsWith('smali'),
  );

  for (const dir of smaliDirs) {
    const fullPath = path.join(decodedDir, dir, relativePath);
    if (fs.existsSync(fullPath)) {
      return fullPath;
    }
  }

  throw new Error(
    `Smali file not found for ${activityName}. Searched in: ` +
    smaliDirs.map((d) => path.join(decodedDir, d, relativePath)).join(', '),
  );
}

/**
 * Patch a smali file to add System.loadLibrary("jig") to its <clinit>.
 *
 * If a <clinit> method exists, the load call is inserted at the start
 * (after .locals/.registers). If no <clinit> exists, one is created.
 */
export function patchSmali(smaliPath: string): void {
  const LOAD_INSTRUCTION =
    '    const-string v0, "jig"\n' +
    '    invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V';

  let content = fs.readFileSync(smaliPath, 'utf-8');

  // Check if already patched
  if (content.includes('const-string v0, "jig"')) {
    return;
  }

  const clinitIndex = content.indexOf('.method static constructor <clinit>()V');

  if (clinitIndex !== -1) {
    // <clinit> exists — insert after .locals or .registers line
    const afterClinit = content.substring(clinitIndex);
    const localsMatch = afterClinit.match(/(\s*\.(?:locals|registers)\s+)(\d+)/);

    if (localsMatch) {
      const localsLine = localsMatch[0];
      const currentCount = parseInt(localsMatch[2], 10);
      const newCount = Math.max(currentCount, 1); // need at least 1 register

      // Replace .locals count and inject load after it
      const insertPoint = clinitIndex + afterClinit.indexOf(localsLine) + localsLine.length;
      const updatedLocalsLine = localsLine.replace(/\d+/, String(newCount));

      content =
        content.substring(0, clinitIndex + afterClinit.indexOf(localsLine)) +
        updatedLocalsLine + '\n\n' +
        LOAD_INSTRUCTION + '\n' +
        content.substring(insertPoint);
    } else {
      // No .locals line — insert right after the method signature line
      const endOfMethodLine = content.indexOf('\n', clinitIndex);
      content =
        content.substring(0, endOfMethodLine + 1) +
        '    .locals 1\n\n' +
        LOAD_INSTRUCTION + '\n' +
        content.substring(endOfMethodLine + 1);
    }
  } else {
    // No <clinit> — create one before the first .method or at end of file
    const clinitBlock =
      '\n.method static constructor <clinit>()V\n' +
      '    .locals 1\n\n' +
      LOAD_INSTRUCTION + '\n\n' +
      '    return-void\n' +
      '.end method\n';

    const firstMethodIndex = content.indexOf('\n.method ');
    if (firstMethodIndex !== -1) {
      content =
        content.substring(0, firstMethodIndex) +
        clinitBlock +
        content.substring(firstMethodIndex);
    } else {
      content += clinitBlock;
    }
  }

  fs.writeFileSync(smaliPath, content, 'utf-8');
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
