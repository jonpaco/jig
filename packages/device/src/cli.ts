#!/usr/bin/env node
import { Command } from 'commander';
import {
  bootDevice,
  killDevice,
  killAll,
  loadManifest,
  checkDependencies,
  formatCheckResult,
  resolveProfile,
  listActiveDevices,
} from './index';
import { ANDROID_DEFAULTS, IOS_DEFAULTS, Platform } from './types';
import { setVerbose } from './log';

const program = new Command();

program
  .name('jig-device')
  .description('Deterministic emulator & simulator management')
  .version('0.1.0')
  .option('-v, --verbose', 'Enable verbose logging (same as JIG_DEBUG=1)')
  .hook('preAction', () => {
    const opts = program.opts();
    if (opts.verbose) setVerbose(true);
  });

program
  .command('check [profile]')
  .description('Validate dependencies for a device profile (or all profiles)')
  .option('--manifest <path>', 'Path to jig.devices.yml')
  .action(async (profileName?: string, opts?: { manifest?: string }) => {
    try {
      const manifest = loadManifest(opts?.manifest);
      if (!manifest) {
        console.error('✗ No manifest found. Create jig.devices.yml or pass --manifest.');
        process.exit(1);
      }

      const profilesToCheck = profileName
        ? { [profileName]: resolveProfile(manifest, profileName) }
        : manifest.devices;

      let allOk = true;
      for (const [name, profile] of Object.entries(profilesToCheck)) {
        console.log(`\nChecking ${name}:`);
        const result = await checkDependencies(profile);
        console.log(formatCheckResult(result));
        if (!result.ok) allOk = false;
      }

      if (!allOk) process.exit(1);
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : String(err);
      console.error(`✗ ${message}`);
      process.exit(1);
    }
  });

program
  .command('boot <profile>')
  .description('Boot a device by profile name')
  .option('--manifest <path>', 'Path to jig.devices.yml')
  .option('-t, --timeout <ms>', 'Boot timeout in ms', '120000')
  .action(async (profileName: string, opts: any) => {
    try {
      const handle = await bootDevice({
        profile: profileName,
        manifest: opts.manifest,
        timeout: parseInt(opts.timeout, 10),
      });
      console.log(`✓ Device ready (${handle.id})`);
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : String(err);
      console.error(`✗ ${message}`);
      process.exit(1);
    }
  });

program
  .command('list')
  .description('List running jig-managed devices')
  .action(() => {
    const devices = listActiveDevices();
    if (devices.length === 0) {
      console.log('No jig-managed devices running.');
      return;
    }
    for (const d of devices) {
      const age = Math.round((Date.now() - new Date(d.bootedAt).getTime()) / 1000);
      console.log(`  ${d.name} (${d.platform}) — ${d.id} — ${age}s`);
    }
  });

program
  .command('kill [profile]')
  .description('Kill a device by profile name')
  .option('--all', 'Kill all jig-managed devices')
  .action(async (profileName?: string, opts?: { all?: boolean }) => {
    try {
      if (opts?.all) {
        await killAll();
        console.log('✓ All devices killed.');
      } else if (profileName) {
        await killDevice(profileName);
        console.log(`✓ ${profileName} killed.`);
      } else {
        console.error('Specify a profile name or --all');
        process.exit(1);
      }
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : String(err);
      console.error(`✗ ${message}`);
      process.exit(1);
    }
  });

program.parse();
