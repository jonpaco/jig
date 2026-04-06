import { getPackageAndActivity, findOrCreateDebugKeystore } from '../src/android/inject';

describe('getPackageAndActivity', () => {
  it('rejects when APK does not exist', async () => {
    await expect(
      getPackageAndActivity('/nonexistent.apk'),
    ).rejects.toThrow();
  });
});

describe('findOrCreateDebugKeystore', () => {
  it('returns a path to a keystore', async () => {
    const ks = await findOrCreateDebugKeystore();
    expect(typeof ks).toBe('string');
    expect(ks).toContain('debug.keystore');
  });
});
