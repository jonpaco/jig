#!/usr/bin/env node
import { Command } from 'commander';
import { status } from './commands/status';

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
      });
      console.log(output);
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : String(err);
      console.error(`✗ ${message}`);
      process.exit(1);
    }
  });

program.parse();
