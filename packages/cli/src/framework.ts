import fs from 'fs';
import path from 'path';
import crypto from 'crypto';

/**
 * Locate the Jig.framework binary for injection.
 *
 * Resolution order:
 * 1. Explicit path (--framework flag)
 * 2. packages/cli/binaries/ios/Jig.framework (npm distribution)
 * 3. packages/native/build/Jig.framework (local monorepo development)
 */
export async function resolveFramework(explicitPath?: string): Promise<string> {
  if (explicitPath) {
    const binaryPath = explicitPath.endsWith('Jig')
      ? explicitPath
      : path.join(explicitPath, 'Jig');
    if (!fs.existsSync(binaryPath)) {
      throw new Error(`Framework binary not found at ${binaryPath}`);
    }
    return binaryPath;
  }

  // Distribution path: relative to this file's package
  const cliRoot = path.resolve(__dirname, '..');
  const distPath = path.join(cliRoot, 'binaries', 'ios', 'Jig.framework', 'Jig');
  if (fs.existsSync(distPath)) {
    return distPath;
  }

  // Development path: sibling native package
  const devPath = path.resolve(cliRoot, '..', 'native', 'build', 'Jig.framework', 'Jig');
  if (fs.existsSync(devPath)) {
    return devPath;
  }

  throw new Error(
    'Jig.framework not found. Looked in:\n' +
    `  ${distPath}\n` +
    `  ${devPath}\n` +
    'Run "pnpm build:framework" in packages/native/ or pass --framework <path>.'
  );
}

/**
 * Verify the binary's SHA-256 hash against the manifest.
 */
export async function verifyBinaryIntegrity(
  binaryPath: string,
  manifestPath: string,
): Promise<void> {
  if (!fs.existsSync(manifestPath)) {
    throw new Error(`Binary integrity manifest not found at ${manifestPath}`);
  }

  const manifest = fs.readFileSync(manifestPath, 'utf-8');
  const binaryName = 'Jig.framework/Jig';
  const expectedLine = manifest
    .split('\n')
    .find((line) => line.trim().endsWith(binaryName));

  if (!expectedLine) {
    throw new Error(`No entry for ${binaryName} in manifest`);
  }

  const expectedHash = expectedLine.trim().split(/\s+/)[0];

  const content = fs.readFileSync(binaryPath);
  const actualHash = crypto.createHash('sha256').update(content).digest('hex');

  if (actualHash !== expectedHash) {
    throw new Error(
      `Binary integrity check failed for ${binaryName}.\n` +
      `  Expected: ${expectedHash}\n` +
      `  Actual:   ${actualHash}`
    );
  }
}
