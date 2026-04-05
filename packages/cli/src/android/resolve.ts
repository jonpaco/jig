import fs from 'fs';
import path from 'path';

/**
 * Locate the pre-built libjig.so directory containing per-ABI subdirectories.
 *
 * Resolution order:
 *   1. Explicit path (--libjig flag)
 *   2. packages/cli/binaries/android/ (npm distribution)
 *   3. packages/native/build/android/ (local monorepo development)
 */
export function resolveLibjigDir(explicitPath?: string): string {
  if (explicitPath) {
    if (!fs.existsSync(explicitPath)) {
      throw new Error(`libjig directory not found at ${explicitPath}`);
    }
    return explicitPath;
  }

  // Distribution path
  const cliRoot = path.resolve(__dirname, '..', '..');
  const distPath = path.join(cliRoot, 'binaries', 'android');
  if (fs.existsSync(distPath)) {
    return distPath;
  }

  // Development path
  const devPath = path.resolve(cliRoot, '..', 'native', 'build', 'android');
  if (fs.existsSync(devPath)) {
    return devPath;
  }

  throw new Error(
    'libjig.so directory not found. Looked in:\n' +
    `  ${distPath}\n` +
    `  ${devPath}\n` +
    'Run "scripts/build-so.sh" to build, or pass --libjig <path>.',
  );
}
