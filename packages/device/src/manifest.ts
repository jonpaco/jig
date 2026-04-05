import { parse, stringify } from 'yaml';
import {
  DeviceManifest,
  DeviceProfile,
  Platform,
  ANDROID_DEFAULTS,
  IOS_DEFAULTS,
} from './types';

/**
 * Parse a YAML manifest string into a DeviceManifest.
 * Applies platform-specific defaults for any missing fields.
 */
export function parseManifest(yamlContent: string): DeviceManifest {
  const raw = parse(yamlContent) as { devices?: Record<string, any> };

  if (!raw?.devices || typeof raw.devices !== 'object') {
    throw new Error('Manifest must have a "devices" key with named profiles');
  }

  const devices: Record<string, DeviceProfile> = {};

  for (const [name, rawProfile] of Object.entries(raw.devices)) {
    if (!rawProfile?.platform) {
      throw new Error(
        `Profile "${name}" is missing required "platform" field (android or ios)`,
      );
    }

    const platform = rawProfile.platform as string;

    if (platform === 'android') {
      devices[name] = {
        platform: 'android',
        api: rawProfile.api ?? ANDROID_DEFAULTS.api,
        arch: rawProfile.arch ?? ANDROID_DEFAULTS.arch,
        image: rawProfile.image ?? ANDROID_DEFAULTS.image,
        screen: rawProfile.screen ?? ANDROID_DEFAULTS.screen,
        ram: rawProfile.ram ?? ANDROID_DEFAULTS.ram,
        headless: rawProfile.headless ?? ANDROID_DEFAULTS.headless,
      };
    } else if (platform === 'ios') {
      devices[name] = {
        platform: 'ios',
        runtime: rawProfile.runtime ?? IOS_DEFAULTS.runtime,
        device: rawProfile.device ?? IOS_DEFAULTS.device,
        headless: rawProfile.headless ?? IOS_DEFAULTS.headless,
      };
    } else {
      throw new Error(
        `Profile "${name}" has unknown platform "${platform}". Expected "android" or "ios".`,
      );
    }
  }

  return { devices };
}

/**
 * Resolve a named profile from a manifest.
 */
export function resolveProfile(manifest: DeviceManifest, name: string): DeviceProfile {
  const profile = manifest.devices[name];
  if (!profile) {
    const available = Object.keys(manifest.devices).join(', ');
    throw new Error(
      `Profile "${name}" not found in manifest. Available: ${available}`,
    );
  }
  return profile;
}

/**
 * Generate a default manifest YAML string for a given platform.
 * Used when jig launch auto-boots a device and no manifest exists.
 */
export function generateManifest(platform: Platform): string {
  const profileName = platform === 'android' ? 'android-default' : 'ios-default';

  const profile = platform === 'android'
    ? { platform: 'android', ...ANDROID_DEFAULTS }
    : { platform: 'ios', ...IOS_DEFAULTS };

  const manifest = { devices: { [profileName]: profile } };
  return stringify(manifest, { lineWidth: 0 });
}
