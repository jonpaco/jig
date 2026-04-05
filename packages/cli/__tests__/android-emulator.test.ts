import {
  getConnectedDevices,
  ensureDevice,
} from '../src/android/emulator';

// Mock @jig/device module
jest.mock('@jig/device', () => ({
  bootDevice: jest.fn(),
  loadManifest: jest.fn().mockReturnValue(null),
  ANDROID_DEFAULTS: {
    avdName: 'jig-emulator',
    api: 34,
    headless: true,
  },
}));

// Mock the internal getConnectedDevices re-exported from @jig/device
jest.mock('@jig/device/dist/android/emulator', () => ({
  getConnectedDevices: jest.fn(),
}));

import { bootDevice, loadManifest } from '@jig/device';
import { getConnectedDevices as mockGetDevices } from '@jig/device/dist/android/emulator';

const mockBootDevice = bootDevice as jest.Mock;
const mockLoadManifest = loadManifest as jest.Mock;
const mockGetConnectedDevices = mockGetDevices as jest.Mock;

describe('getConnectedDevices', () => {
  afterEach(() => jest.resetAllMocks());

  it('returns empty array when no devices connected', async () => {
    mockGetConnectedDevices.mockResolvedValue([]);
    const devices = await getConnectedDevices();
    expect(devices).toEqual([]);
  });

  it('returns device serials when devices are connected', async () => {
    mockGetConnectedDevices.mockResolvedValue(['emulator-5554', 'R5CT12345']);
    const devices = await getConnectedDevices();
    expect(devices).toEqual(['emulator-5554', 'R5CT12345']);
  });
});

describe('ensureDevice', () => {
  afterEach(() => jest.resetAllMocks());

  it('returns immediately when a device is already connected', async () => {
    mockGetConnectedDevices.mockResolvedValue(['emulator-5554']);
    const result = await ensureDevice();
    expect(result.alreadyRunning).toBe(true);
    expect(result.serial).toBe('emulator-5554');
    expect(mockBootDevice).not.toHaveBeenCalled();
  });

  it('boots a device when none connected (no manifest)', async () => {
    mockGetConnectedDevices.mockResolvedValue([]);
    mockLoadManifest.mockReturnValue(null);
    mockBootDevice.mockResolvedValue({ id: 'emulator-5554' });

    const result = await ensureDevice();
    expect(result.alreadyRunning).toBe(false);
    expect(result.booted).toBe(true);
    expect(result.serial).toBe('emulator-5554');
  });

  it('uses manifest profile when available', async () => {
    mockGetConnectedDevices.mockResolvedValue([]);
    mockLoadManifest.mockReturnValue({
      devices: {
        'ci-android': { platform: 'android', headless: true },
      },
    });
    mockBootDevice.mockResolvedValue({ id: 'emulator-5556' });

    const result = await ensureDevice();
    expect(mockBootDevice).toHaveBeenCalledWith(
      expect.objectContaining({ profile: 'ci-android' }),
    );
    expect(result.serial).toBe('emulator-5556');
  });

  it('falls back to defaults when profile boot fails', async () => {
    mockGetConnectedDevices.mockResolvedValue([]);
    mockLoadManifest.mockReturnValue(null);
    mockBootDevice
      .mockRejectedValueOnce(new Error('profile not found'))
      .mockResolvedValueOnce({ id: 'emulator-5558' });

    const result = await ensureDevice();
    expect(mockBootDevice).toHaveBeenCalledTimes(2);
    expect(result.serial).toBe('emulator-5558');
    expect(result.booted).toBe(true);
  });
});
