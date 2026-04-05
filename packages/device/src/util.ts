import { execFile, ExecFileOptions } from 'child_process';
import { promisify } from 'util';
import { debug } from './log';

const rawExecFile = promisify(execFile);

/**
 * Execute a command and log it in verbose mode.
 * Logs the command, stdout (truncated), stderr, and duration.
 */
export async function execFileAsync(
  cmd: string,
  args: string[] = [],
  opts?: ExecFileOptions,
): Promise<{ stdout: string; stderr: string }> {
  const cmdStr = [cmd, ...args].join(' ');
  debug(`exec: ${cmdStr}`);
  const start = Date.now();

  try {
    const result = await rawExecFile(cmd, args, opts);
    const ms = Date.now() - start;
    if (result.stdout) {
      const lines = String(result.stdout).split('\n');
      const preview = lines.length > 3
        ? lines.slice(0, 3).join('\n') + `\n  ... (${lines.length} lines)`
        : String(result.stdout).trim();
      debug(`  stdout: ${preview}`);
    }
    if (result.stderr) {
      debug(`  stderr: ${String(result.stderr).trim().slice(0, 200)}`);
    }
    debug(`  exit: 0 (${ms}ms)`);
    return { stdout: String(result.stdout), stderr: String(result.stderr) };
  } catch (err: any) {
    const ms = Date.now() - start;
    debug(`  exit: ${err.code || 'error'} (${ms}ms)`);
    if (err.stderr) debug(`  stderr: ${String(err.stderr).trim().slice(0, 500)}`);
    throw err;
  }
}

export function sleep(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms));
}
