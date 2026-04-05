import { buildIOSChecks } from '../src/check';

describe('buildIOSChecks', () => {
  it('includes Xcode check', () => {
    const checks = buildIOSChecks({
      platform: 'ios',
      runtime: '18.0',
      device: 'iPhone 16',
      headless: true,
    });
    const names = checks.map((c) => c.name);
    expect(names).toContain('Xcode');
    expect(names).toContain('simctl');
    expect(names).toContain('runtime');
    expect(names).toContain('device type');
  });
});
