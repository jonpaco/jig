/**
 * Verbose logger for jig-device.
 *
 * Enabled by:
 *   - JIG_DEBUG=1 environment variable
 *   - --verbose CLI flag (sets process.env.JIG_DEBUG)
 *
 * All output goes to stderr so it doesn't interfere with
 * structured stdout (device IDs, JSON, etc.).
 */

let verbose = !!(process.env.JIG_DEBUG || process.env.JIG_VERBOSE);

export function setVerbose(enabled: boolean): void {
  verbose = enabled;
}

export function isVerbose(): boolean {
  return verbose;
}

/**
 * Always printed — key lifecycle events.
 */
export function info(message: string): void {
  process.stderr.write(`  ${message}\n`);
}

/**
 * Only printed in verbose mode — command details, decisions, timing.
 */
export function debug(message: string): void {
  if (verbose) {
    process.stderr.write(`  [debug] ${message}\n`);
  }
}

/**
 * Always printed — non-fatal warnings.
 */
export function warn(message: string): void {
  process.stderr.write(`  [warn] ${message}\n`);
}

/**
 * Log a timed operation. Returns the result and logs duration.
 */
export async function timed<T>(label: string, fn: () => Promise<T>): Promise<T> {
  const start = Date.now();
  debug(`${label}...`);
  try {
    const result = await fn();
    const ms = Date.now() - start;
    debug(`${label} — done (${ms}ms)`);
    return result;
  } catch (err) {
    const ms = Date.now() - start;
    debug(`${label} — failed (${ms}ms)`);
    throw err;
  }
}
