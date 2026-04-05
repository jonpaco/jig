import fs from 'fs';
import path from 'path';
import os from 'os';
import { ActiveDevice, DeviceHandle } from './types';

const ACTIVE_DIR = path.join(os.tmpdir(), 'jig-device');
const ACTIVE_FILE = path.join(ACTIVE_DIR, 'active.json');

function readActive(): ActiveDevice[] {
  try {
    const content = fs.readFileSync(ACTIVE_FILE, 'utf-8');
    return JSON.parse(content);
  } catch {
    return [];
  }
}

function writeActive(devices: ActiveDevice[]): void {
  fs.mkdirSync(ACTIVE_DIR, { recursive: true });
  fs.writeFileSync(ACTIVE_FILE, JSON.stringify(devices, null, 2));
}

/**
 * Check if a process is alive.
 */
function isProcessAlive(pid: number): boolean {
  try {
    process.kill(pid, 0);
    return true;
  } catch {
    return false;
  }
}

/**
 * Register a booted device.
 */
export function registerDevice(handle: DeviceHandle, pid?: number): void {
  const devices = readActive().filter((d) => d.id !== handle.id);
  devices.push({
    id: handle.id,
    platform: handle.platform,
    name: handle.name,
    port: handle.port,
    pid,
    bootedAt: new Date().toISOString(),
  });
  writeActive(devices);
}

/**
 * Unregister a device.
 */
export function unregisterDevice(id: string): void {
  const devices = readActive().filter((d) => d.id !== id);
  writeActive(devices);
}

/**
 * List all active devices, cleaning up stale entries.
 */
export function listActiveDevices(): ActiveDevice[] {
  const devices = readActive();
  const alive = devices.filter((d) => {
    if (d.pid && !isProcessAlive(d.pid)) return false;
    return true;
  });
  // Clean up stale entries
  if (alive.length !== devices.length) {
    writeActive(alive);
  }
  return alive;
}

/**
 * Find an active device by profile name.
 */
export function findActiveDevice(name: string): ActiveDevice | undefined {
  return listActiveDevices().find((d) => d.name === name);
}
