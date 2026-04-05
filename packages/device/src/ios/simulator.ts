import { execFileAsync, sleep } from '../util';
import { info, debug, timed } from '../log';
import { IOSProfile, DeviceHandle } from '../types';

const SIM_PREFIX = 'jig-';

interface SimctlDevice {
  udid: string;
  name: string;
  state: string;
  isAvailable: boolean;
}

/**
 * List booted iOS simulators.
 */
export async function getBootedSimulators(): Promise<string[]> {
  try {
    const { stdout } = await execFileAsync('xcrun', [
      'simctl', 'list', 'devices', 'booted', '-j',
    ]);
    const data = JSON.parse(stdout);
    const udids: string[] = [];
    for (const runtime of Object.values(data.devices) as SimctlDevice[][]) {
      for (const device of runtime) {
        if (device.state === 'Booted') {
          udids.push(device.udid);
        }
      }
    }
    return udids;
  } catch {
    return [];
  }
}

/**
 * Find an existing jig-managed simulator by profile name.
 */
async function findExistingSimulator(profileName: string): Promise<string | null> {
  const simName = `${SIM_PREFIX}${profileName}`;
  try {
    const { stdout } = await execFileAsync('xcrun', [
      'simctl', 'list', 'devices', '-j',
    ]);
    const data = JSON.parse(stdout);
    for (const runtime of Object.values(data.devices) as SimctlDevice[][]) {
      for (const device of runtime) {
        if (device.name === simName && device.isAvailable) {
          return device.udid;
        }
      }
    }
  } catch {
    // simctl not available
  }
  return null;
}

/**
 * Resolve the runtime identifier string for simctl.
 * 'latest' resolves to the highest installed runtime.
 */
async function resolveRuntime(runtime: string): Promise<string> {
  const { stdout } = await execFileAsync('xcrun', [
    'simctl', 'list', 'runtimes', '-j',
  ]);
  const data = JSON.parse(stdout);
  const runtimes: Array<{ identifier: string; name: string; version: string; isAvailable: boolean }> =
    data.runtimes.filter((r: any) => r.isAvailable && r.name.startsWith('iOS'));

  if (runtimes.length === 0) {
    throw new Error(
      'No iOS runtimes installed.\n' +
      'Install via: Xcode > Settings > Platforms > + > iOS',
    );
  }

  if (runtime === 'latest') {
    runtimes.sort((a, b) => {
      const aParts = a.version.split('.').map(Number);
      const bParts = b.version.split('.').map(Number);
      for (let i = 0; i < Math.max(aParts.length, bParts.length); i++) {
        const diff = (bParts[i] || 0) - (aParts[i] || 0);
        if (diff !== 0) return diff;
      }
      return 0;
    });
    return runtimes[0].identifier;
  }

  const match = runtimes.find((r) => r.version.startsWith(runtime));
  if (!match) {
    const available = runtimes.map((r) => r.version).join(', ');
    throw new Error(
      `iOS runtime "${runtime}" not installed.\n` +
      `Available: ${available}\n` +
      `Install via: Xcode > Settings > Platforms > + > iOS ${runtime}`,
    );
  }

  return match.identifier;
}

/**
 * Resolve the device type identifier for simctl.
 */
async function resolveDeviceType(device: string): Promise<string> {
  const { stdout } = await execFileAsync('xcrun', [
    'simctl', 'list', 'devicetypes', '-j',
  ]);
  const data = JSON.parse(stdout);
  const types: Array<{ identifier: string; name: string }> = data.devicetypes;

  const match = types.find((t) => t.name === device);
  if (!match) {
    const available = types
      .filter((t) => t.name.startsWith('iPhone'))
      .map((t) => t.name)
      .slice(-10);
    throw new Error(
      `Device type "${device}" not found.\n` +
      `Available: ${available.join(', ')}`,
    );
  }

  return match.identifier;
}

/**
 * Boot an iOS simulator from a profile.
 */
export async function bootIOS(
  profileName: string,
  profile: IOSProfile,
  timeout: number = 60_000,
): Promise<DeviceHandle> {
  const simName = `${SIM_PREFIX}${profileName}`;
  debug(`Profile "${profileName}": runtime=${profile.runtime}, device=${profile.device}, headless=${profile.headless}`);

  let udid = await findExistingSimulator(profileName);

  if (!udid) {
    const runtimeId = await timed('Resolve runtime', () => resolveRuntime(profile.runtime));
    debug(`Runtime resolved: ${runtimeId}`);
    const deviceTypeId = await timed('Resolve device type', () => resolveDeviceType(profile.device));
    debug(`Device type resolved: ${deviceTypeId}`);

    const { stdout } = await execFileAsync('xcrun', [
      'simctl', 'create', simName, deviceTypeId, runtimeId,
    ]);
    udid = stdout.trim();
    info(`Created simulator ${simName} (${udid})`);
  } else {
    debug(`Reusing existing simulator ${simName} (${udid})`);
  }

  // Erase for determinism
  try {
    await execFileAsync('xcrun', ['simctl', 'erase', udid]);
  } catch {
    // May fail if already booted
  }

  // Boot
  try {
    await execFileAsync('xcrun', ['simctl', 'boot', udid]);
  } catch (err) {
    const msg = err instanceof Error ? err.message : String(err);
    if (!msg.includes('Booted')) throw err;
  }

  // Wait for boot
  try {
    await execFileAsync('xcrun', ['simctl', 'bootstatus', udid, '-b'], {
      timeout: Math.max(timeout, 1000),
    });
  } catch {
    // Best effort
  }

  // Open Simulator.app if not headless
  if (!profile.headless) {
    try {
      await execFileAsync('open', ['-a', 'Simulator']);
    } catch {
      // Best effort
    }
  }

  // Disable keyboard autocorrect for determinism
  await disableKeyboardFeatures(udid);

  info(`Simulator ready (${udid})`);

  return {
    id: udid,
    platform: 'ios',
    name: profileName,
  };
}

/**
 * Kill an iOS simulator by UDID.
 */
export async function killIOS(udid: string): Promise<void> {
  try {
    await execFileAsync('xcrun', ['simctl', 'shutdown', udid], { timeout: 10_000 });
  } catch {
    // Best effort
  }
}

async function disableKeyboardFeatures(udid: string): Promise<void> {
  const prefs: Record<string, string> = {
    KeyboardAutocorrection: 'false',
    KeyboardPrediction: 'false',
    KeyboardShowPredictionBar: 'false',
  };
  for (const [key, value] of Object.entries(prefs)) {
    try {
      await execFileAsync('xcrun', [
        'simctl', 'spawn', udid, 'defaults', 'write',
        'com.apple.Preferences', key, '-bool', value,
      ]);
    } catch {
      // Best effort
    }
  }
}
