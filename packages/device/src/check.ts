import fs from 'fs';
import { execFileAsync } from './util';
import { AndroidProfile, IOSProfile, DeviceProfile, CheckResult, CheckEntry } from './types';
import { resolveSDKTool, resolveAbi, findSystemImage, findBestSystemImage } from './android/sdk';

export async function checkDependencies(profile: DeviceProfile): Promise<CheckResult> {
  if (profile.platform === 'android') {
    return runAndroidChecks(profile);
  }
  // iOS checks added in Plan 2
  return { ok: true, checks: [] };
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
  return checks;
}

async function runAndroidChecks(profile: AndroidProfile): Promise<CheckResult> {
  const checks: CheckEntry[] = [];

  // ANDROID_HOME
  const androidHome = process.env.ANDROID_HOME || process.env.ANDROID_SDK_ROOT;
  if (androidHome && fs.existsSync(androidHome)) {
    checks.push({ name: 'ANDROID_HOME', passed: true, detail: androidHome });
  } else {
    checks.push({
      name: 'ANDROID_HOME',
      passed: false,
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
        checks.push({ name: 'emulator', passed: true, detail: version });
      } catch {
        checks.push({ name: 'emulator', passed: true, detail: emulatorPath });
      }
    } else {
      checks.push({
        name: 'emulator', passed: false,
        message: `Android Emulator not found at ${emulatorPath}`,
        fix: 'sdkmanager "emulator"',
      });
    }
  } catch (err) {
    checks.push({
      name: 'emulator', passed: false,
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
        checks.push({ name: 'adb', passed: true, detail: version });
      } catch {
        checks.push({ name: 'adb', passed: true, detail: adbPath });
      }
    } else {
      checks.push({
        name: 'adb', passed: false,
        message: `adb not found at ${adbPath}`,
        fix: 'sdkmanager "platform-tools"',
      });
    }
  } catch (err) {
    checks.push({
      name: 'adb', passed: false,
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
      checks.push({ name: 'system image', passed: true, detail: found });
    } else {
      const best = await findBestSystemImage(abi);
      if (best) {
        checks.push({
          name: 'system image', passed: true,
          detail: `${best} (exact match not found, will use this)`,
        });
      } else {
        checks.push({
          name: 'system image', passed: false,
          message: `"${imageId}" not installed`,
          fix: `sdkmanager "${imageId}"`,
        });
      }
    }
  } catch {
    checks.push({
      name: 'system image', passed: false,
      message: `Could not check for "${imageId}"`,
      fix: `sdkmanager "${imageId}"`,
    });
  }

  // Hardware acceleration
  const accelCheck = await checkHardwareAcceleration();
  checks.push(accelCheck);

  // libpulse0 (Linux only)
  if (process.platform === 'linux') {
    checks.push(checkLibpulse());
  }

  const ok = checks.every((c) => c.passed);
  return { ok, checks };
}

async function checkHardwareAcceleration(): Promise<CheckEntry> {
  if (process.platform === 'linux') {
    try {
      fs.accessSync('/dev/kvm', fs.constants.R_OK | fs.constants.W_OK);
      return { name: 'hardware acceleration', passed: true, detail: 'KVM' };
    } catch {
      return {
        name: 'hardware acceleration', passed: false,
        message: 'KVM not available at /dev/kvm or not writable',
        fix: 'Enable virtualization in BIOS, then: sudo apt install qemu-kvm && sudo usermod -aG kvm $USER',
      };
    }
  }

  if (process.platform === 'darwin') {
    try {
      const { stdout } = await execFileAsync('sysctl', ['kern.hv_support']);
      if (stdout.includes('1')) {
        return { name: 'hardware acceleration', passed: true, detail: 'HVF' };
      }
      return {
        name: 'hardware acceleration', passed: false,
        message: 'Hypervisor.framework not supported on this Mac',
        fix: 'HVF requires Apple Silicon or an Intel Mac with VT-x. GitHub Actions macOS runners may not expose HVF.',
      };
    } catch {
      return {
        name: 'hardware acceleration', passed: false,
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
        return { name: 'hardware acceleration', passed: true, detail: stdout.trim() };
      }
      return {
        name: 'hardware acceleration', passed: false,
        message: stdout.trim(),
        fix: 'Install Intel HAXM or enable Windows Hypervisor Platform in Windows Features',
      };
    } catch {
      return {
        name: 'hardware acceleration', passed: false,
        message: 'Could not run emulator -accel-check',
        fix: 'Install Intel HAXM or enable Windows Hypervisor Platform in Windows Features',
      };
    }
  }

  return { name: 'hardware acceleration', passed: true, detail: 'unknown platform' };
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
      return { name: 'libpulse0', passed: true, detail: p };
    }
  }
  return {
    name: 'libpulse0', passed: false,
    message: 'libpulse.so.0 not found (Android emulator requires it even with -no-audio)',
    fix: 'sudo apt-get install -y libpulse0',
  };
}

export function formatCheckResult(result: CheckResult): string {
  const lines: string[] = [];
  for (const check of result.checks) {
    if (check.passed) {
      lines.push(`  ✓ ${check.name}: ${check.detail || 'ok'}`);
    } else {
      lines.push(`  ✗ ${check.name}: ${check.message}`);
      if (check.fix) {
        lines.push(`    Install via: ${check.fix}`);
      }
    }
  }
  const failCount = result.checks.filter((c) => !c.passed).length;
  if (failCount === 0) {
    lines.push('  All checks passed.');
  } else {
    lines.push(`  ${failCount} issue${failCount === 1 ? '' : 's'} found. Fix and re-run.`);
  }
  return lines.join('\n');
}
