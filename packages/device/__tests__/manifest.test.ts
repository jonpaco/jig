import { parseManifest, resolveProfile, generateManifest } from '../src/manifest';
import { ANDROID_DEFAULTS, IOS_DEFAULTS } from '../src/types';

describe('parseManifest', () => {
  it('parses a full android profile', () => {
    const yaml = `
devices:
  android-ci:
    platform: android
    api: 34
    arch: x86_64
    image: google_apis
    screen: 1080x2400
    ram: 4096
    headless: true
`;
    const manifest = parseManifest(yaml);
    expect(manifest.devices['android-ci']).toEqual({
      platform: 'android',
      api: 34,
      arch: 'x86_64',
      image: 'google_apis',
      screen: '1080x2400',
      ram: 4096,
      headless: true,
    });
  });

  it('applies android defaults for missing fields', () => {
    const yaml = `
devices:
  quick:
    platform: android
`;
    const manifest = parseManifest(yaml);
    expect(manifest.devices['quick']).toEqual({
      platform: 'android',
      ...ANDROID_DEFAULTS,
    });
  });

  it('applies ios defaults for missing fields', () => {
    const yaml = `
devices:
  sim:
    platform: ios
`;
    const manifest = parseManifest(yaml);
    expect(manifest.devices['sim']).toEqual({
      platform: 'ios',
      ...IOS_DEFAULTS,
    });
  });

  it('throws on missing platform field', () => {
    const yaml = `
devices:
  bad:
    api: 34
`;
    expect(() => parseManifest(yaml)).toThrow(/platform/);
  });

  it('throws on unknown platform', () => {
    const yaml = `
devices:
  bad:
    platform: windows
`;
    expect(() => parseManifest(yaml)).toThrow(/platform/);
  });
});

describe('resolveProfile', () => {
  it('returns the named profile', () => {
    const manifest = parseManifest(`
devices:
  android-ci:
    platform: android
    api: 34
`);
    const profile = resolveProfile(manifest, 'android-ci');
    expect(profile.platform).toBe('android');
  });

  it('throws on unknown profile name', () => {
    const manifest = parseManifest(`
devices:
  android-ci:
    platform: android
`);
    expect(() => resolveProfile(manifest, 'nope')).toThrow(/nope/);
  });
});

describe('generateManifest', () => {
  it('generates yaml with android defaults', () => {
    const yaml = generateManifest('android');
    expect(yaml).toContain('platform: android');
    expect(yaml).toContain('api: 34');
    expect(yaml).toContain('headless: true');
  });

  it('generates yaml with ios defaults', () => {
    const yaml = generateManifest('ios');
    expect(yaml).toContain('platform: ios');
    expect(yaml).toContain('iPhone 16');
  });
});
