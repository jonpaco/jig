import fs from 'fs';
import path from 'path';
import {
  DeviceHandle,
  DeviceProfile,
  DeviceManifest,
  BootOptions,
  CheckResult,
  Platform,
  ActiveDevice,
  ANDROID_DEFAULTS,
  IOS_DEFAULTS,
} from './types';
import { parseManifest, resolveProfile, generateManifest } from './manifest';
import { checkDependencies, formatCheckResult } from './check';
import { bootAndroid, killAndroid, getConnectedDevices } from './android/emulator';
import { resolveAbi } from './android/sdk';
import { registerDevice, unregisterDevice, listActiveDevices, findActiveDevice } from './active';
import { debug, info, setVerbose } from './log';

export {
  // Logging
  setVerbose,
  // Types
  DeviceHandle,
  DeviceProfile,
  DeviceManifest,
  BootOptions,
  CheckResult,
  Platform,
  ActiveDevice,
  ANDROID_DEFAULTS,
  IOS_DEFAULTS,
  // Manifest
  parseManifest,
  resolveProfile,
  generateManifest,
  // Check
  checkDependencies,
  formatCheckResult,
  // Active tracking
  listActiveDevices,
  findActiveDevice,
};

const DEFAULT_MANIFEST_NAME = 'jig.devices.yml';

/**
 * Load a manifest from a file path. Returns null if not found.
 */
export function loadManifest(manifestPath?: string): DeviceManifest | null {
  const filePath = manifestPath || DEFAULT_MANIFEST_NAME;
  const resolved = path.isAbsolute(filePath) ? filePath : path.resolve(filePath);

  if (!fs.existsSync(resolved)) {
    debug(`Manifest not found at ${resolved}`);
    return null;
  }

  debug(`Loading manifest from ${resolved}`);
  const content = fs.readFileSync(resolved, 'utf-8');
  const manifest = parseManifest(content);
  debug(`Manifest loaded: ${Object.keys(manifest.devices).join(', ')}`);
  return manifest;
}

/**
 * Boot a device from a profile.
 */
export async function bootDevice(options: BootOptions = {}): Promise<DeviceHandle> {
  const { profile: profileName, manifest: manifestPath, timeout = 120_000 } = options;

  let resolvedProfile: DeviceProfile;
  let name: string;

  if (profileName) {
    const manifest = loadManifest(manifestPath);
    if (!manifest) {
      throw new Error(
        `Manifest not found at ${manifestPath || DEFAULT_MANIFEST_NAME}. ` +
        `Create one or use --platform/--api flags.`,
      );
    }
    resolvedProfile = resolveProfile(manifest, profileName);
    name = profileName;
  } else if (options.config) {
    // Inline config (from CLI flags)
    const config = options.config;
    if (!config.platform) {
      throw new Error('--platform is required when not using a manifest profile');
    }
    if (config.platform === 'android') {
      resolvedProfile = { ...ANDROID_DEFAULTS, ...config, platform: 'android' };
    } else {
      resolvedProfile = { ...IOS_DEFAULTS, ...config, platform: 'ios' };
    }
    name = `inline-${config.platform}`;
  } else {
    throw new Error('Specify a --profile name or --platform flag');
  }

  if (resolvedProfile.platform === 'android') {
    const handle = await bootAndroid(name, resolvedProfile, timeout);
    registerDevice(handle);
    return handle;
  }

  if (resolvedProfile.platform === 'ios') {
    const { bootIOS } = await import('./ios/simulator');
    const handle = await bootIOS(name, resolvedProfile, timeout);
    registerDevice(handle);
    return handle;
  }

  throw new Error(`Unknown platform: ${(resolvedProfile as any).platform}`);
}

/**
 * Kill a device by profile name or handle.
 */
export async function killDevice(nameOrHandle: string | DeviceHandle): Promise<void> {
  if (typeof nameOrHandle === 'string') {
    const active = findActiveDevice(nameOrHandle);
    if (!active) {
      throw new Error(`No active device found with name "${nameOrHandle}"`);
    }
    if (active.platform === 'android') {
      await killAndroid(active.id);
    }
    if (active.platform === 'ios') {
      const { killIOS } = await import('./ios/simulator');
      await killIOS(active.id);
    }
    unregisterDevice(active.id);
  } else {
    if (nameOrHandle.platform === 'android') {
      await killAndroid(nameOrHandle.id);
    }
    if (nameOrHandle.platform === 'ios') {
      const { killIOS } = await import('./ios/simulator');
      await killIOS(nameOrHandle.id);
    }
    unregisterDevice(nameOrHandle.id);
  }
}

/**
 * Kill all jig-managed devices.
 */
export async function killAll(): Promise<void> {
  const devices = listActiveDevices();
  for (const device of devices) {
    if (device.platform === 'android') {
      await killAndroid(device.id);
    }
    if (device.platform === 'ios') {
      const { killIOS } = await import('./ios/simulator');
      await killIOS(device.id);
    }
    unregisterDevice(device.id);
  }
}
