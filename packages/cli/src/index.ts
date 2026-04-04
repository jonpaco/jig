#!/usr/bin/env node
import { Command } from 'commander';
import { status } from './commands/status';
import { wait } from './commands/wait';
import { launch } from './commands/launch';

const program = new Command();

program
  .name('jig')
  .description('Jig CLI — programmatic control of running React Native apps')
  .version('0.1.0');

program
  .command('status')
  .description('Check connection to a running Jig-enabled app')
  .option('-H, --host <host>', 'Host to connect to', 'localhost')
  .option('-p, --port <port>', 'Port to connect to', '4042')
  .option('-t, --timeout <ms>', 'Connection timeout in ms', '5000')
  .action(async (opts) => {
    try {
      const output = await status({
        host: opts.host,
        port: parseInt(opts.port, 10),
        timeout: parseInt(opts.timeout, 10),
        clientName: '@jig/cli',
        clientVersion: '0.1.0',
      });
      console.log(output);
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : String(err);
      console.error(`✗ ${message}`);
      process.exit(1);
    }
  });

program
  .command('wait')
  .description('Wait for a Jig-enabled app to become reachable')
  .option('-H, --host <host>', 'Host to connect to', 'localhost')
  .option('-p, --port <port>', 'Port to connect to', '4042')
  .option('-t, --timeout <ms>', 'Total timeout in ms', '30000')
  .option('-i, --interval <ms>', 'Retry interval in ms', '1000')
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
      const message = err instanceof Error ? err.message : String(err);
      console.error(`✗ ${message}`);
      process.exit(1);
    }
  });

program
  .command('launch')
  .description('Install and launch an app on iOS simulator with Jig injected')
  .argument('<app-path>', 'Path to the .app bundle')
  .option('--framework <path>', 'Path to Jig.framework (auto-detected if omitted)')
  .option('-p, --port <port>', 'Jig server port', '4042')
  .option('--skip-verify', 'Skip SHA-256 integrity check', false)
  .action(async (appPath: string, opts) => {
    try {
      const output = await launch({
        appPath,
        framework: opts.framework,
        port: parseInt(opts.port, 10),
        skipVerify: opts.skipVerify,
      });
      console.log(output);
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : String(err);
      console.error(`✗ ${message}`);
      process.exit(1);
    }
  });

program.parse();
