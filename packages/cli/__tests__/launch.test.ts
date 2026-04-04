import { extractBundleId, buildLaunchEnv } from '../src/commands/launch';
import fs from 'fs';
import path from 'path';
import os from 'os';

describe('extractBundleId', () => {
  it('extracts CFBundleIdentifier from an Info.plist', async () => {
    const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'jig-test-'));
    const appDir = path.join(tmpDir, 'Test.app');
    fs.mkdirSync(appDir);

    const plist = {
      CFBundleIdentifier: 'com.test.myapp',
      CFBundleName: 'TestApp',
    };
    fs.writeFileSync(path.join(appDir, 'Info.plist'), JSON.stringify(plist));

    const bundleId = await extractBundleId(appDir);
    expect(bundleId).toBe('com.test.myapp');

    fs.rmSync(tmpDir, { recursive: true });
  });

  it('throws if Info.plist is missing', async () => {
    const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'jig-test-'));
    await expect(extractBundleId(tmpDir)).rejects.toThrow('Info.plist');
    fs.rmSync(tmpDir, { recursive: true });
  });
});

describe('buildLaunchEnv', () => {
  it('sets SIMCTL_CHILD_DYLD_INSERT_LIBRARIES to framework binary path', () => {
    const env = buildLaunchEnv('/path/to/Jig.framework/Jig');
    expect(env.SIMCTL_CHILD_DYLD_INSERT_LIBRARIES).toBe('/path/to/Jig.framework/Jig');
  });
});
