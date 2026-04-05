import { execFile } from 'child_process';
import { promisify } from 'util';
import fs from 'fs';
import path from 'path';
import { resolveFramework, verifyBinaryIntegrity } from '../framework';
import { launchAndroid } from '../android/launch';

const execFileAsync = promisify(execFile);

export interface LaunchOptions {
  appPath: string;
  framework?: string;
  libjig?: string;
  port?: number;
  skipVerify?: boolean;
}

/**
 * Extract CFBundleIdentifier from an .app bundle's Info.plist.
 * Uses plutil to handle both binary and XML plist formats.
 */
export async function extractBundleId(appPath: string): Promise<string> {
  const plistPath = path.join(appPath, 'Info.plist');

  if (!fs.existsSync(plistPath)) {
    throw new Error(`Info.plist not found at ${plistPath}`);
  }

  // Try plutil first (handles binary plists)
  try {
    const { stdout } = await execFileAsync('plutil', [
      '-convert', 'json', '-o', '-', plistPath,
    ]);
    const plist = JSON.parse(stdout);
    const bundleId = plist.CFBundleIdentifier;
    if (!bundleId) {
      throw new Error('CFBundleIdentifier not found in Info.plist');
    }
    return bundleId;
  } catch (err) {
    // Fallback: try reading as JSON directly (for tests with JSON plists)
    try {
      const content = fs.readFileSync(plistPath, 'utf-8');
      const plist = JSON.parse(content);
      if (plist.CFBundleIdentifier) {
        return plist.CFBundleIdentifier;
      }
    } catch {
      // ignore fallback errors
    }
    throw err instanceof Error ? err : new Error(String(err));
  }
}

/**
 * Build the environment variables for launching with injection.
 */
export function buildLaunchEnv(frameworkBinaryPath: string): Record<string, string> {
  return {
    SIMCTL_CHILD_DYLD_INSERT_LIBRARIES: frameworkBinaryPath,
  };
}

/**
 * Find the UDID of a booted simulator.
 */
async function findBootedSimulator(): Promise<string> {
  const { stdout } = await execFileAsync('xcrun', [
    'simctl', 'list', 'devices', 'booted', '-j',
  ]);
  const data = JSON.parse(stdout);
  for (const runtime of Object.values(data.devices) as any[][]) {
    for (const device of runtime) {
      if (device.state === 'Booted') {
        return device.udid;
      }
    }
  }
  throw new Error(
    'No booted iOS simulator found. Boot one with:\n' +
    '  xcrun simctl boot "iPhone 16"'
  );
}

/**
 * Install and launch an app on the booted simulator with Jig injected.
 */
export async function launch(options: LaunchOptions): Promise<string> {
  const { appPath, framework, libjig, port = 4042, skipVerify = false } = options;

  if (!fs.existsSync(appPath)) {
    throw new Error(`App not found at ${appPath}`);
  }

  // Android path: .apk files
  if (appPath.endsWith('.apk')) {
    return launchAndroid({ apkPath: appPath, libjig, port });
  }

  // iOS path: .app bundles
  // Resolve and verify framework
  const frameworkBinary = await resolveFramework(framework);
  const frameworkDir = path.dirname(frameworkBinary);
  const manifestPath = path.join(path.dirname(frameworkDir), 'jig-binaries.sha256');

  if (!skipVerify && fs.existsSync(manifestPath)) {
    await verifyBinaryIntegrity(frameworkBinary, manifestPath);
  }

  // Extract bundle ID
  const bundleId = await extractBundleId(appPath);

  // Find booted simulator
  const udid = await findBootedSimulator();

  // Install app
  await execFileAsync('xcrun', ['simctl', 'install', udid, appPath]);

  // Launch with injection
  const env = buildLaunchEnv(frameworkBinary);
  await execFileAsync('xcrun', [
    'simctl', 'launch', udid, bundleId,
  ], {
    env: { ...process.env, ...env },
  });

  return (
    `Launched ${path.basename(appPath)} (${bundleId}) on simulator ${udid}\n` +
    `Jig server starting on port ${port}...`
  );
}
