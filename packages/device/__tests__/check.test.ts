import { buildAndroidChecks, formatCheckResult } from '../src/check';
import { CheckResult } from '../src/types';

describe('buildAndroidChecks', () => {
  it('includes ANDROID_HOME check', () => {
    const checks = buildAndroidChecks({
      platform: 'android',
      api: 34,
      arch: 'x86_64',
      image: 'google_apis',
      screen: '1080x2400',
      ram: 4096,
      headless: true,
    });
    const names = checks.map((c) => c.name);
    expect(names).toContain('ANDROID_HOME');
    expect(names).toContain('emulator');
    expect(names).toContain('adb');
    expect(names).toContain('system image');
  });

  it('includes KVM check on linux', () => {
    const checks = buildAndroidChecks({
      platform: 'android',
      api: 34,
      arch: 'x86_64',
      image: 'google_apis',
      screen: '1080x2400',
      ram: 4096,
      headless: true,
    }, 'linux');
    const names = checks.map((c) => c.name);
    expect(names).toContain('hardware acceleration');
  });

  it('includes HVF check on darwin', () => {
    const checks = buildAndroidChecks({
      platform: 'android',
      api: 34,
      arch: 'x86_64',
      image: 'google_apis',
      screen: '1080x2400',
      ram: 4096,
      headless: true,
    }, 'darwin');
    const names = checks.map((c) => c.name);
    expect(names).toContain('hardware acceleration');
  });
});

describe('formatCheckResult', () => {
  it('formats passing result', () => {
    const result: CheckResult = {
      ok: true,
      checks: [
        { name: 'ANDROID_HOME', passed: true, detail: '/fake/sdk' },
      ],
    };
    const output = formatCheckResult(result);
    expect(output).toContain('ANDROID_HOME');
    expect(output).toContain('/fake/sdk');
    expect(output).toContain('All checks passed');
  });

  it('formats failing result with fix', () => {
    const result: CheckResult = {
      ok: false,
      checks: [
        { name: 'adb', passed: false, message: 'adb not found', fix: 'sdkmanager "platform-tools"' },
      ],
    };
    const output = formatCheckResult(result);
    expect(output).toContain('adb not found');
    expect(output).toContain('sdkmanager "platform-tools"');
    expect(output).toContain('1 issue');
  });
});
