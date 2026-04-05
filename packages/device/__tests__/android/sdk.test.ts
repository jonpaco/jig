import { getHostAbi, resolveSDKTool, parseImageString } from '../../src/android/sdk';

describe('getHostAbi', () => {
  it('maps arm64 to arm64-v8a', () => {
    expect(getHostAbi('arm64')).toBe('arm64-v8a');
  });

  it('maps x64 to x86_64', () => {
    expect(getHostAbi('x64')).toBe('x86_64');
  });

  it('throws on unsupported arch', () => {
    expect(() => getHostAbi('ia32')).toThrow(/Unsupported/);
  });
});

describe('resolveSDKTool', () => {
  it('throws when ANDROID_HOME is not set', () => {
    const originalHome = process.env.ANDROID_HOME;
    const originalRoot = process.env.ANDROID_SDK_ROOT;
    delete process.env.ANDROID_HOME;
    delete process.env.ANDROID_SDK_ROOT;
    try {
      expect(() => resolveSDKTool('emulator')).toThrow(/ANDROID_HOME/);
    } finally {
      if (originalHome) process.env.ANDROID_HOME = originalHome;
      if (originalRoot) process.env.ANDROID_SDK_ROOT = originalRoot;
    }
  });

  it('returns correct path for emulator', () => {
    const originalHome = process.env.ANDROID_HOME;
    process.env.ANDROID_HOME = '/fake/sdk';
    try {
      const result = resolveSDKTool('emulator');
      expect(result).toBe('/fake/sdk/emulator/emulator');
    } finally {
      if (originalHome) process.env.ANDROID_HOME = originalHome;
      else delete process.env.ANDROID_HOME;
    }
  });
});

describe('parseImageString', () => {
  it('parses sdkmanager output', () => {
    const output = `  system-images;android-34;google_apis;x86_64
  system-images;android-33;default;arm64-v8a`;
    const images = parseImageString(output);
    expect(images).toHaveLength(2);
    expect(images[0]).toEqual({
      full: 'system-images;android-34;google_apis;x86_64',
      api: 34,
      target: 'google_apis',
      arch: 'x86_64',
    });
  });

  it('returns empty array for no matches', () => {
    expect(parseImageString('nothing here')).toEqual([]);
  });
});
