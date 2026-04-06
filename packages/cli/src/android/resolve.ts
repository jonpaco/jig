import fs from 'fs';
import path from 'path';

/** CLI package root (packages/cli/) */
const cliRoot = path.resolve(__dirname, '..', '..');

/** Native package root (packages/native/) — sibling in monorepo */
const nativeRoot = path.resolve(cliRoot, '..', 'native');

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

  const candidates = [
    path.join(cliRoot, 'binaries', 'android'),
    path.join(nativeRoot, 'build', 'android'),
  ];

  for (const p of candidates) {
    if (fs.existsSync(p)) return p;
  }

  throw new Error(
    'libjig.so directory not found. Looked in:\n' +
    candidates.map((p) => `  ${p}`).join('\n') + '\n' +
    'Run "scripts/build-so.sh" to build, or pass --libjig <path>.',
  );
}

/**
 * Locate jig-helpers.dex (compiled Java helpers for APK injection).
 *
 * Resolution order:
 *   1. packages/cli/binaries/jig-helpers.dex (npm distribution)
 *   2. packages/native/build/dex/jig-helpers.dex (local monorepo development)
 */
export function resolveHelpersDex(): string {
  const candidates = [
    path.join(cliRoot, 'binaries', 'jig-helpers.dex'),
    path.join(nativeRoot, 'build', 'dex', 'jig-helpers.dex'),
  ];

  for (const p of candidates) {
    if (fs.existsSync(p)) return p;
  }

  throw new Error(
    'jig-helpers.dex not found. Looked in:\n' +
    candidates.map((p) => `  ${p}`).join('\n') + '\n' +
    'Run "pnpm build:dex" in @jig/native to build.',
  );
}

/**
 * Locate jig-dex-patcher.jar (DEX clinit patching tool).
 *
 * Resolution order:
 *   1. packages/cli/binaries/jig-dex-patcher.jar (npm distribution)
 *   2. packages/native/android/dex-patcher/build/libs/jig-dex-patcher.jar (local monorepo development)
 */
export function resolveDexPatcher(): string {
  const candidates = [
    path.join(cliRoot, 'binaries', 'jig-dex-patcher.jar'),
    path.join(nativeRoot, 'android', 'dex-patcher', 'build', 'libs', 'jig-dex-patcher.jar'),
  ];

  for (const p of candidates) {
    if (fs.existsSync(p)) return p;
  }

  throw new Error(
    'jig-dex-patcher.jar not found. Looked in:\n' +
    candidates.map((p) => `  ${p}`).join('\n') + '\n' +
    'Run "pnpm build:dex-patcher" in @jig/native to build.',
  );
}
