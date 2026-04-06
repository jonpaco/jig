#!/usr/bin/env node
import { Command } from 'commander';
import { status } from './commands/status';
import { wait } from './commands/wait';
import { launch } from './commands/launch';
import { DEFAULT_PORT, DEFAULT_CONNECT_TIMEOUT, DEFAULT_WAIT_TIMEOUT, DEFAULT_RETRY_INTERVAL, CLIENT_VERSION } from '@jig/protocol';

const program = new Command();

program
  .name('jig')
  .description('Jig CLI — programmatic control of running React Native apps')
  .version('0.1.0');

program
  .command('status')
  .description('Check connection to a running Jig-enabled app')
  .option('-H, --host <host>', 'Host to connect to', 'localhost')
  .option('-p, --port <port>', 'Port to connect to', String(DEFAULT_PORT))
  .option('-t, --timeout <ms>', 'Connection timeout in ms', String(DEFAULT_CONNECT_TIMEOUT))
  .action(async (opts) => {
    try {
      const output = await status({
        host: opts.host,
        port: parseInt(opts.port, 10),
        timeout: parseInt(opts.timeout, 10),
        clientName: '@jig/cli',
        clientVersion: CLIENT_VERSION,
      });
      console.log(output);
    } catch (err: unknown) {
      handleError(err);
    }
  });

program
  .command('wait')
  .description('Wait for a Jig-enabled app to become reachable')
  .option('-H, --host <host>', 'Host to connect to', 'localhost')
  .option('-p, --port <port>', 'Port to connect to', String(DEFAULT_PORT))
  .option('-t, --timeout <ms>', 'Total timeout in ms', String(DEFAULT_WAIT_TIMEOUT))
  .option('-i, --interval <ms>', 'Retry interval in ms', String(DEFAULT_RETRY_INTERVAL))
  .action(async (opts) => {
    try {
      const output = await wait({
        host: opts.host,
        port: parseInt(opts.port, 10),
        timeout: parseInt(opts.timeout, 10),
        interval: parseInt(opts.interval, 10),
      });
      console.log(output);
    } catch (err: unknown) {
      handleError(err);
    }
  });

program
  .command('launch')
  .description('Install and launch an app with Jig injected (.app for iOS, .apk for Android)')
  .argument('<app-path>', 'Path to the .app bundle or .apk file')
  .option('--framework <path>', 'Path to Jig.framework for iOS (auto-detected if omitted)')
  .option('--libjig <path>', 'Path to libjig.so directory for Android (auto-detected if omitted)')
  .option('-p, --port <port>', 'Jig server port', String(DEFAULT_PORT))
  .option('--skip-verify', 'Skip SHA-256 integrity check', false)
  .option('--device <profile>', 'Boot a device from jig.devices.yml profile before launching')
  .option('--no-device', 'Skip auto-booting a device if none is running')
  .action(async (appPath: string, opts) => {
    try {
      const output = await launch({
        appPath,
        framework: opts.framework,
        libjig: opts.libjig,
        port: parseInt(opts.port, 10),
        skipVerify: opts.skipVerify,
        deviceProfile: opts.device === false ? undefined : opts.device,
        noDevice: opts.device === false,
      });
      console.log(output);
    } catch (err: unknown) {
      handleError(err);
    }
  });

function handleError(err: unknown): never {
  const message = err instanceof Error ? err.message : String(err);
  console.error(`✗ ${message}`);
  process.exit(1);
}

program.parse();
