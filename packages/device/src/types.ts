/** Platform for a device profile. */
export type Platform = 'android' | 'ios';

/** Android device profile from manifest. */
export interface AndroidProfile {
  platform: 'android';
  api: number;
  arch: 'auto' | 'x86_64' | 'arm64-v8a';
  image: string;
  screen: string;
  ram: number;
  headless: boolean;
}

/** iOS device profile from manifest. */
export interface IOSProfile {
  platform: 'ios';
  runtime: string;
  device: string;
  headless: boolean;
}

/** A named device profile from the manifest. */
export type DeviceProfile = AndroidProfile | IOSProfile;

/** Android defaults. */
export const ANDROID_DEFAULTS: Omit<AndroidProfile, 'platform'> = {
  api: 34,
  arch: 'auto',
  image: 'google_apis',
  screen: '1080x2400',
  ram: 4096,
  headless: true,
};

/** iOS defaults. */
export const IOS_DEFAULTS: Omit<IOSProfile, 'platform'> = {
  runtime: 'latest',
  device: 'iPhone 16',
  headless: true,
};

/** Handle to a running device, returned by bootDevice(). */
export interface DeviceHandle {
  id: string;         // emulator-5554 or simulator UDID
  platform: Platform;
  name: string;       // profile name
  port?: number;      // emulator port (Android only)
}

/** Result of dependency check. */
export interface CheckResult {
  ok: boolean;
  checks: CheckEntry[];
}

export interface CheckEntry {
  name: string;
  passed: boolean;
  detail?: string;    // version or path when passed
  message?: string;   // error message when failed
  fix?: string;       // install command or link
}

/** Parsed manifest. */
export interface DeviceManifest {
  devices: Record<string, DeviceProfile>;
}

/** Options for bootDevice(). */
export interface BootOptions {
  profile?: string;
  manifest?: string;
  config?: Partial<AndroidProfile> | Partial<IOSProfile>;
  timeout?: number;
}

/** Active device record stored in active.json. */
export interface ActiveDevice {
  id: string;
  platform: Platform;
  name: string;
  port?: number;
  pid?: number;
  bootedAt: string;   // ISO timestamp
}
