import {
  getConnectedDevices,
  findBestSystemImage,
  getHostArch,
  ensureDevice,
} from '../src/android/emulator';

jest.mock('child_process', () => ({
  execFile: jest.fn(),
}));

import { execFile } from 'child_process';
const mockExecFile = execFile as unknown as jest.Mock;

function mockExecFileResult(stdout: string) {
  mockExecFile.mockImplementation(
    (_cmd: string, _args: string[], cb: (err: Error | null, result: { stdout: string }) => void) => {
      cb(null, { stdout });
    },
  );
}

function mockExecFileError(message: string) {
  mockExecFile.mockImplementation(
    (_cmd: string, _args: string[], cb: (err: Error | null) => void) => {
      cb(new Error(message));
    },
  );
}

describe('getConnectedDevices', () => {
  afterEach(() => jest.resetAllMocks());

  it('returns empty array when no devices connected', async () => {
    mockExecFileResult('List of devices attached\n\n');
    const devices = await getConnectedDevices();
    expect(devices).toEqual([]);
  });

  it('returns device serials when devices are connected', async () => {
    mockExecFileResult(
      'List of devices attached\nemulator-5554\tdevice\nR5CT12345\tdevice\n\n',
    );
    const devices = await getConnectedDevices();
    expect(devices).toEqual(['emulator-5554', 'R5CT12345']);
  });

  it('ignores devices in offline or unauthorized state', async () => {
    mockExecFileResult(
      'List of devices attached\nemulator-5554\toffline\nR5CT12345\tdevice\n\n',
    );
    const devices = await getConnectedDevices();
    expect(devices).toEqual(['R5CT12345']);
  });

  it('returns empty array when adb fails', async () => {
    mockExecFileError('adb not found');
    const devices = await getConnectedDevices();
    expect(devices).toEqual([]);
  });
});

describe('getHostArch', () => {
  it('returns arm64-v8a for arm64 hosts', () => {
    expect(getHostArch('arm64')).toBe('arm64-v8a');
  });

  it('returns x86_64 for x64 hosts', () => {
    expect(getHostArch('x64')).toBe('x86_64');
  });

  it('throws for unsupported architectures', () => {
    expect(() => getHostArch('ia32')).toThrow('Unsupported host architecture');
  });
});

describe('findBestSystemImage', () => {
  afterEach(() => jest.resetAllMocks());

  it('selects highest API google_apis image matching arch', async () => {
    mockExecFileResult(`Installed packages:
  system-images;android-31;google_apis;arm64-v8a
  system-images;android-34;google_apis;arm64-v8a
  system-images;android-34;default;arm64-v8a
  system-images;android-30;google_apis;x86_64
`);
    const image = await findBestSystemImage('arm64-v8a');
    expect(image).toBe('system-images;android-34;google_apis;arm64-v8a');
  });

  it('falls back to default target when google_apis unavailable', async () => {
    mockExecFileResult(`Installed packages:
  system-images;android-33;default;arm64-v8a
`);
    const image = await findBestSystemImage('arm64-v8a');
    expect(image).toBe('system-images;android-33;default;arm64-v8a');
  });

  it('falls back to google_apis_playstore', async () => {
    mockExecFileResult(`Installed packages:
  system-images;android-33;google_apis_playstore;arm64-v8a
`);
    const image = await findBestSystemImage('arm64-v8a');
    expect(image).toBe('system-images;android-33;google_apis_playstore;arm64-v8a');
  });

  it('returns null when no matching image found', async () => {
    mockExecFileResult(`Installed packages:
  system-images;android-34;google_apis;x86_64
`);
    const image = await findBestSystemImage('arm64-v8a');
    expect(image).toBeNull();
  });

  it('handles table-format sdkmanager output', async () => {
    mockExecFileResult(`  Path                                        | Version | Description
  -------                                     | ------- | -------
  system-images;android-34;google_apis;arm64-v8a | 14      | Google APIs ARM 64 v8a System Image
`);
    const image = await findBestSystemImage('arm64-v8a');
    expect(image).toBe('system-images;android-34;google_apis;arm64-v8a');
  });
});

describe('ensureDevice', () => {
  afterEach(() => jest.resetAllMocks());

  it('returns immediately when a device is already connected', async () => {
    mockExecFileResult('List of devices attached\nemulator-5554\tdevice\n\n');
    const result = await ensureDevice();
    expect(result.alreadyRunning).toBe(true);
    expect(result.serial).toBe('emulator-5554');
  });
});
