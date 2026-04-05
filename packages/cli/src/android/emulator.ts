/**
 * Android emulator lifecycle — delegates to @jig/device.
 *
 * This file is a thin wrapper that maintains the existing interface
 * (`ensureDevice()`) while delegating to the standalone @jig/device package.
 */

import {
  bootDevice,
  loadManifest,
  ANDROID_DEFAULTS,
} from '@jig/device';
import { getConnectedDevices as getDevices } from '@jig/device/dist/android/emulator';

export { getDevices as getConnectedDevices };

export const JIG_AVD_NAME = 'jig-emulator';

export interface EnsureDeviceResult {
  serial: string;
  alreadyRunning: boolean;
  booted: boolean;
}

/**
 * Ensure an Android device or emulator is available.
 * If none connected, boots one via @jig/device.
 */
export async function ensureDevice(): Promise<EnsureDeviceResult> {
  const devices = await getDevices();
  if (devices.length > 0) {
    return { serial: devices[0], alreadyRunning: true, booted: false };
  }

  // Try manifest first
  const manifest = loadManifest();
  let profileName = 'android-ci';

  if (manifest) {
    // Find first headless android profile
    for (const [name, profile] of Object.entries(manifest.devices)) {
      if (profile.platform === 'android' && profile.headless) {
        profileName = name;
        break;
      }
    }
  }

  try {
    const handle = await bootDevice({
      profile: profileName,
      timeout: 120_000,
    });
    return { serial: handle.id, alreadyRunning: false, booted: true };
  } catch {
    // Fallback: boot with defaults if manifest profile not found
    const handle = await bootDevice({
      config: { platform: 'android', ...ANDROID_DEFAULTS },
      timeout: 120_000,
    });

    const fs = await import('fs');
    const { generateManifest } = await import('@jig/device');
    const manifestPath = 'jig.devices.yml';
    if (!fs.existsSync(manifestPath)) {
      fs.writeFileSync(manifestPath, generateManifest('android'));
      process.stderr.write('Created jig.devices.yml with default profile. Edit to customize.\n');
    }

    return { serial: handle.id, alreadyRunning: false, booted: true };
  }
}
