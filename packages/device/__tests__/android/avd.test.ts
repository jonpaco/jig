import { buildConfigIni, buildPointerIni, AVD_PREFIX } from '../../src/android/avd';

describe('buildConfigIni', () => {
  it('generates valid config for arm64-v8a', () => {
    const config = buildConfigIni({
      profileName: 'android-ci',
      systemImage: 'system-images;android-34;google_apis;arm64-v8a',
      ram: 4096,
      screen: '1080x2400',
    });
    expect(config).toContain('abi.type=arm64-v8a');
    expect(config).toContain('hw.cpu.arch=arm64');
    expect(config).toContain('hw.ramSize=4096');
    expect(config).toContain('hw.lcd.width=1080');
    expect(config).toContain('hw.lcd.height=2400');
    expect(config).toContain('hw.gpu.enabled=yes');
    expect(config).toContain('target=android-34');
    expect(config).toContain('tag.id=google_apis');
  });

  it('generates valid config for x86_64', () => {
    const config = buildConfigIni({
      profileName: 'android-ci',
      systemImage: 'system-images;android-34;google_apis;x86_64',
      ram: 4096,
      screen: '1080x2400',
    });
    expect(config).toContain('abi.type=x86_64');
    expect(config).toContain('hw.cpu.arch=x86_64');
  });

  it('handles google_apis_playstore target', () => {
    const config = buildConfigIni({
      profileName: 'test',
      systemImage: 'system-images;android-34;google_apis_playstore;arm64-v8a',
      ram: 4096,
      screen: '1080x2400',
    });
    expect(config).toContain('PlayStore.enabled=true');
    expect(config).toContain('tag.display=Google Play');
  });
});

describe('buildPointerIni', () => {
  it('generates pointer with correct path', () => {
    const ini = buildPointerIni('/home/user/.android/avd/jig-test.avd', '34');
    expect(ini).toContain('path=/home/user/.android/avd/jig-test.avd');
    expect(ini).toContain('target=android-34');
  });
});

describe('AVD_PREFIX', () => {
  it('is jig-', () => {
    expect(AVD_PREFIX).toBe('jig-');
  });
});
