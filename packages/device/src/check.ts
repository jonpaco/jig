import fs from 'fs';
import path from 'path';
import { execFileAsync } from './util';
import { AndroidProfile, IOSProfile, DeviceProfile, CheckResult, CheckEntry } from './types';
import { resolveSDKTool, resolveAbi, findSystemImage, findBestSystemImage } from './android/sdk';

export async function checkDependencies(profile: DeviceProfile): Promise<CheckResult> {
  if (profile.platform === 'android') {
    return runAndroidChecks(profile);
  }
  if (profile.platform === 'ios') {
    return runIOSChecks(profile);
  }
  return { ok: true, checks: [] };
}

export function buildIOSChecks(
  profile: IOSProfile,
): Array<{ name: string }> {
  return [
    { name: 'Xcode' },
    { name: 'simctl' },
    { name: 'runtime' },
    { name: 'device type' },
  ];
}

async function runIOSChecks(profile: IOSProfile): Promise<CheckResult> {
  const checks: CheckEntry[] = [];

  // Xcode
  try {
    const { stdout } = await execFileAsync('xcode-select', ['-p']);
    const xcodePath = stdout.trim();
    if (xcodePath) {
      checks.push({ name: 'Xcode', passed: true, severity: 'error', detail: xcodePath });
    } else {
      checks.push({
        name: 'Xcode', passed: false, severity: 'error',
        message: 'Xcode developer tools not found',
        fix: 'Install Xcode from the App Store, then run: xcode-select --install',
      });
    }
  } catch {
    checks.push({
      name: 'Xcode', passed: false, severity: 'error',
      message: 'xcode-select not available or Xcode not installed',
      fix: 'Install Xcode from the App Store, then run: xcode-select --install',
    });
  }

  // simctl
  let simctlRuntimes: Array<{ identifier: string; name: string; version: string; isAvailable: boolean }> = [];

  try {
    const { stdout } = await execFileAsync('xcrun', ['simctl', 'list', 'runtimes', '-j']);
    const data = JSON.parse(stdout);
    simctlRuntimes = (data.runtimes || []).filter((r: any) => r.isAvailable && r.name.startsWith('iOS'));
    checks.push({ name: 'simctl', passed: true, severity: 'error', detail: 'available' });
  } catch {
    checks.push({
      name: 'simctl', passed: false, severity: 'error',
      message: 'xcrun simctl not available',
      fix: 'Install Xcode and accept the license: sudo xcodebuild -license accept',
    });
  }

  // runtime
  if (simctlRuntimes.length > 0) {
    if (profile.runtime === 'latest') {
      const best = simctlRuntimes.sort((a, b) => {
        const aParts = a.version.split('.').map(Number);
        const bParts = b.version.split('.').map(Number);
        for (let i = 0; i < Math.max(aParts.length, bParts.length); i++) {
          const diff = (bParts[i] || 0) - (aParts[i] || 0);
          if (diff !== 0) return diff;
        }
        return 0;
      })[0];
      checks.push({ name: 'runtime', passed: true, severity: 'error', detail: `${best.name} (latest)` });
    } else {
      const match = simctlRuntimes.find((r) => r.version.startsWith(profile.runtime));
      if (match) {
        checks.push({ name: 'runtime', passed: true, severity: 'error', detail: match.name });
      } else {
        const available = simctlRuntimes.map((r) => r.version).join(', ');
        checks.push({
          name: 'runtime', passed: false, severity: 'error',
          message: `iOS runtime "${profile.runtime}" not installed. Available: ${available}`,
          fix: `Install via: Xcode > Settings > Platforms > + > iOS ${profile.runtime}`,
        });
      }
    }
  } else {
    const failedSimctl = checks.find((c) => c.name === 'simctl' && !c.passed);
    if (!failedSimctl) {
      checks.push({
        name: 'runtime', passed: false, severity: 'error',
        message: 'No iOS runtimes installed',
        fix: 'Install via: Xcode > Settings > Platforms > + > iOS',
      });
    } else {
      checks.push({
        name: 'runtime', passed: false, severity: 'error',
        message: 'Cannot check runtimes — simctl not available',
      });
    }
  }

  // device type
  try {
    const { stdout } = await execFileAsync('xcrun', ['simctl', 'list', 'devicetypes', '-j']);
    const data = JSON.parse(stdout);
    const deviceTypes = data.devicetypes || [];
    const match = deviceTypes.find((t: any) => t.name === profile.device);
    if (match) {
      checks.push({ name: 'device type', passed: true, severity: 'error', detail: match.identifier });
    } else {
      const available = deviceTypes
        .filter((t: any) => t.name.startsWith('iPhone'))
        .map((t: any) => t.name)
        .slice(-10)
        .join(', ');
      checks.push({
        name: 'device type', passed: false, severity: 'error',
        message: `Device type "${profile.device}" not found`,
        fix: `Available device types include: ${available}`,
      });
    }
  } catch {
    checks.push({
      name: 'device type', passed: false, severity: 'error',
      message: 'Cannot check device types — simctl not available',
      fix: 'Install Xcode and accept the license: sudo xcodebuild -license accept',
    });
  }

  // ok = no errors (warnings don't block)
  const ok = checks.filter((c) => c.severity === 'error').every((c) => c.passed);
  return { ok, checks };
}

export function buildAndroidChecks(
  profile: AndroidProfile,
  platform: string = process.platform,
): Array<{ name: string }> {
  const checks: Array<{ name: string }> = [
    { name: 'ANDROID_HOME' },
    { name: 'emulator' },
    { name: 'adb' },
    { name: 'system image' },
    { name: 'hardware acceleration' },
  ];
  if (platform === 'linux') {
    checks.push({ name: 'libpulse0' });
  }
  // Injection tools (warnings)
  checks.push(
    { name: 'apktool' },
    { name: 'zipalign' },
    { name: 'apksigner' },
    { name: 'aapt2' },
    { name: 'keytool' },
  );
  return checks;
}

async function runAndroidChecks(profile: AndroidProfile): Promise<CheckResult> {
  const checks: CheckEntry[] = [];

  // --- Emulator dependencies (errors) ---

  // ANDROID_HOME
  const androidHome = process.env.ANDROID_HOME || process.env.ANDROID_SDK_ROOT;
  if (androidHome && fs.existsSync(androidHome)) {
    checks.push({ name: 'ANDROID_HOME', passed: true, severity: 'error', detail: androidHome });
  } else {
    checks.push({
      name: 'ANDROID_HOME', passed: false, severity: 'error',
      message: 'ANDROID_HOME not set or directory does not exist',
      fix: 'Install Android Studio or set ANDROID_HOME to your SDK path:\n  https://developer.android.com/studio',
    });
  }

  // emulator
  try {
    const emulatorPath = resolveSDKTool('emulator');
    if (fs.existsSync(emulatorPath)) {
      try {
        const { stdout } = await execFileAsync(emulatorPath, ['-version']);
        const version = stdout.split('\n')[0]?.trim() || 'installed';
        checks.push({ name: 'emulator', passed: true, severity: 'error', detail: version });
      } catch {
        checks.push({ name: 'emulator', passed: true, severity: 'error', detail: emulatorPath });
      }
    } else {
      checks.push({
        name: 'emulator', passed: false, severity: 'error',
        message: `Android Emulator not found at ${emulatorPath}`,
        fix: 'sdkmanager "emulator"',
      });
    }
  } catch (err) {
    checks.push({
      name: 'emulator', passed: false, severity: 'error',
      message: err instanceof Error ? err.message : String(err),
      fix: 'sdkmanager "emulator"',
    });
  }

  // adb
  try {
    const adbPath = resolveSDKTool('adb');
    if (fs.existsSync(adbPath)) {
      try {
        const { stdout } = await execFileAsync(adbPath, ['version']);
        const version = stdout.split('\n')[0]?.trim() || 'installed';
        checks.push({ name: 'adb', passed: true, severity: 'error', detail: version });
      } catch {
        checks.push({ name: 'adb', passed: true, severity: 'error', detail: adbPath });
      }
    } else {
      checks.push({
        name: 'adb', passed: false, severity: 'error',
        message: `adb not found at ${adbPath}`,
        fix: 'sdkmanager "platform-tools"',
      });
    }
  } catch (err) {
    checks.push({
      name: 'adb', passed: false, severity: 'error',
      message: err instanceof Error ? err.message : String(err),
      fix: 'sdkmanager "platform-tools"',
    });
  }

  // System image
  const abi = resolveAbi(profile.arch);
  const imageId = `system-images;android-${profile.api};${profile.image};${abi}`;
  try {
    const found = await findSystemImage(profile.api, profile.image, abi);
    if (found) {
      checks.push({ name: 'system image', passed: true, severity: 'error', detail: found });
    } else {
      const best = await findBestSystemImage(abi);
      if (best) {
        checks.push({
          name: 'system image', passed: true, severity: 'error',
          detail: `${best} (exact match not found, will use this)`,
        });
      } else {
        checks.push({
          name: 'system image', passed: false, severity: 'error',
          message: `"${imageId}" not installed`,
          fix: `sdkmanager "${imageId}"`,
        });
      }
    }
  } catch {
    checks.push({
      name: 'system image', passed: false, severity: 'error',
      message: `Could not check for "${imageId}"`,
      fix: `sdkmanager "${imageId}"`,
    });
  }

  // Hardware acceleration
  checks.push(await checkHardwareAcceleration());

  // libpulse0 (Linux only)
  if (process.platform === 'linux') {
    checks.push(checkLibpulse());
  }

  // --- Injection tools (warnings) ---

  checks.push(...await checkInjectionTools(androidHome));

  // ok = no errors (warnings don't block)
  const ok = checks.filter((c) => c.severity === 'error').every((c) => c.passed);
  return { ok, checks };
}

/**
 * Check injection pipeline tools. These are warnings, not errors —
 * you can boot an emulator without them, but jig launch needs them
 * for APK repackaging.
 */
async function checkInjectionTools(androidHome: string | undefined): Promise<CheckEntry[]> {
  const checks: CheckEntry[] = [];

  // apktool (standalone binary, not part of Android SDK)
  try {
    const { stdout } = await execFileAsync('apktool', ['--version']);
    checks.push({ name: 'apktool', passed: true, severity: 'warning', detail: stdout.trim() });
  } catch {
    checks.push({
      name: 'apktool', passed: false, severity: 'warning',
      message: 'apktool not found (needed for APK injection)',
      fix: 'brew install apktool\n    Or: https://apktool.org/docs/install',
    });
  }

  // Build tools path — zipalign, apksigner, aapt2 all live here
  const buildToolsDir = findBuildToolsDir(androidHome);

  // zipalign
  if (buildToolsDir) {
    const zipalignPath = path.join(buildToolsDir, 'zipalign');
    if (fs.existsSync(zipalignPath)) {
      checks.push({ name: 'zipalign', passed: true, severity: 'warning', detail: buildToolsDir });
    } else {
      checks.push({
        name: 'zipalign', passed: false, severity: 'warning',
        message: 'zipalign not found in build-tools',
        fix: 'sdkmanager "build-tools;35.0.0"',
      });
    }
  } else {
    checks.push({
      name: 'zipalign', passed: false, severity: 'warning',
      message: 'No Android build-tools found',
      fix: 'sdkmanager "build-tools;35.0.0"',
    });
  }

  // apksigner
  if (buildToolsDir) {
    const apksignerPath = path.join(buildToolsDir, 'apksigner');
    if (fs.existsSync(apksignerPath)) {
      checks.push({ name: 'apksigner', passed: true, severity: 'warning', detail: buildToolsDir });
    } else {
      checks.push({
        name: 'apksigner', passed: false, severity: 'warning',
        message: 'apksigner not found in build-tools',
        fix: 'sdkmanager "build-tools;35.0.0"',
      });
    }
  } else {
    checks.push({
      name: 'apksigner', passed: false, severity: 'warning',
      message: 'No Android build-tools found',
      fix: 'sdkmanager "build-tools;35.0.0"',
    });
  }

  // aapt2
  if (buildToolsDir) {
    const aapt2Path = path.join(buildToolsDir, 'aapt2');
    if (fs.existsSync(aapt2Path)) {
      checks.push({ name: 'aapt2', passed: true, severity: 'warning', detail: buildToolsDir });
    } else {
      checks.push({
        name: 'aapt2', passed: false, severity: 'warning',
        message: 'aapt2 not found in build-tools',
        fix: 'sdkmanager "build-tools;35.0.0"',
      });
    }
  } else {
    checks.push({
      name: 'aapt2', passed: false, severity: 'warning',
      message: 'No Android build-tools found',
      fix: 'sdkmanager "build-tools;35.0.0"',
    });
  }

  // keytool (JDK)
  try {
    const { stdout } = await execFileAsync('keytool', ['-help'], { timeout: 5000 });
    checks.push({ name: 'keytool', passed: true, severity: 'warning', detail: 'available' });
  } catch (err: any) {
    // keytool -help exits non-zero but still prints help text — that means it exists
    if (err.stderr?.includes('keytool') || err.stdout?.includes('keytool')) {
      checks.push({ name: 'keytool', passed: true, severity: 'warning', detail: 'available' });
    } else {
      checks.push({
        name: 'keytool', passed: false, severity: 'warning',
        message: 'keytool not found (needed for APK signing)',
        fix: 'Install a JDK: brew install openjdk',
      });
    }
  }

  return checks;
}

/**
 * Find the highest-versioned build-tools directory.
 */
function findBuildToolsDir(androidHome: string | undefined): string | null {
  if (!androidHome) return null;
  const btDir = path.join(androidHome, 'build-tools');
  if (!fs.existsSync(btDir)) return null;

  try {
    const versions = fs.readdirSync(btDir)
      .filter((d) => fs.statSync(path.join(btDir, d)).isDirectory())
      .sort();  // lexicographic sort works for semver-like version dirs
    if (versions.length === 0) return null;
    return path.join(btDir, versions[versions.length - 1]);
  } catch {
    return null;
  }
}

async function checkHardwareAcceleration(): Promise<CheckEntry> {
  if (process.platform === 'linux') {
    try {
      fs.accessSync('/dev/kvm', fs.constants.R_OK | fs.constants.W_OK);
      return { name: 'hardware acceleration', passed: true, severity: 'error', detail: 'KVM' };
    } catch {
      return {
        name: 'hardware acceleration', passed: false, severity: 'error',
        message: 'KVM not available at /dev/kvm or not writable',
        fix: 'Enable virtualization in BIOS, then: sudo apt install qemu-kvm && sudo usermod -aG kvm $USER',
      };
    }
  }

  if (process.platform === 'darwin') {
    try {
      const { stdout } = await execFileAsync('sysctl', ['kern.hv_support']);
      if (stdout.includes('1')) {
        return { name: 'hardware acceleration', passed: true, severity: 'error', detail: 'HVF' };
      }
      return {
        name: 'hardware acceleration', passed: false, severity: 'error',
        message: 'Hypervisor.framework not supported on this Mac',
        fix: 'HVF requires Apple Silicon or an Intel Mac with VT-x. GitHub Actions macOS runners may not expose HVF.',
      };
    } catch {
      return {
        name: 'hardware acceleration', passed: false, severity: 'error',
        message: 'Could not check HVF support',
        fix: 'Run: sysctl kern.hv_support',
      };
    }
  }

  if (process.platform === 'win32') {
    try {
      const emulatorBin = resolveSDKTool('emulator');
      const { stdout } = await execFileAsync(emulatorBin, ['-accel-check']);
      if (stdout.includes('HAXM') || stdout.includes('WHPX') || stdout.includes('is installed')) {
        return { name: 'hardware acceleration', passed: true, severity: 'error', detail: stdout.trim() };
      }
      return {
        name: 'hardware acceleration', passed: false, severity: 'error',
        message: stdout.trim(),
        fix: 'Install Intel HAXM or enable Windows Hypervisor Platform in Windows Features',
      };
    } catch {
      return {
        name: 'hardware acceleration', passed: false, severity: 'error',
        message: 'Could not run emulator -accel-check',
        fix: 'Install Intel HAXM or enable Windows Hypervisor Platform in Windows Features',
      };
    }
  }

  return { name: 'hardware acceleration', passed: true, severity: 'error', detail: 'unknown platform' };
}

function checkLibpulse(): CheckEntry {
  const paths = [
    '/usr/lib/x86_64-linux-gnu/libpulse.so.0',
    '/usr/lib/libpulse.so.0',
    '/usr/lib64/libpulse.so.0',
    '/usr/lib/aarch64-linux-gnu/libpulse.so.0',
  ];
  for (const p of paths) {
    if (fs.existsSync(p)) {
      return { name: 'libpulse0', passed: true, severity: 'error', detail: p };
    }
  }
  return {
    name: 'libpulse0', passed: false, severity: 'error',
    message: 'libpulse.so.0 not found (Android emulator requires it even with -no-audio)',
    fix: 'sudo apt-get install -y libpulse0',
  };
}

export function formatCheckResult(result: CheckResult): string {
  const lines: string[] = [];

  const errors = result.checks.filter((c) => c.severity === 'error');
  const warnings = result.checks.filter((c) => c.severity === 'warning');

  // Emulator/simulator checks
  for (const check of errors) {
    if (check.passed) {
      lines.push(`  ✓ ${check.name}: ${check.detail || 'ok'}`);
    } else {
      lines.push(`  ✗ ${check.name}: ${check.message}`);
      if (check.fix) {
        lines.push(`    Fix: ${check.fix}`);
      }
    }
  }

  // Injection tool checks (only show section if there are any)
  if (warnings.length > 0) {
    const failedWarnings = warnings.filter((c) => !c.passed);
    const passedWarnings = warnings.filter((c) => c.passed);

    if (failedWarnings.length > 0 || passedWarnings.length > 0) {
      lines.push('  Injection tools:');
      for (const check of warnings) {
        if (check.passed) {
          lines.push(`    ✓ ${check.name}: ${check.detail || 'ok'}`);
        } else {
          lines.push(`    ⚠ ${check.name}: ${check.message}`);
          if (check.fix) {
            lines.push(`      Fix: ${check.fix}`);
          }
        }
      }
    }
  }

  // Summary
  const errorCount = errors.filter((c) => !c.passed).length;
  const warnCount = warnings.filter((c) => !c.passed).length;

  if (errorCount === 0 && warnCount === 0) {
    lines.push('  All checks passed.');
  } else if (errorCount === 0 && warnCount > 0) {
    lines.push(`  All checks passed. ${warnCount} warning${warnCount === 1 ? '' : 's'} (injection tools).`);
  } else {
    lines.push(`  ${errorCount} error${errorCount === 1 ? '' : 's'} found. Fix and re-run.`);
    if (warnCount > 0) {
      lines.push(`  ${warnCount} warning${warnCount === 1 ? '' : 's'} (injection tools — not required for boot).`);
    }
  }

  return lines.join('\n');
}
