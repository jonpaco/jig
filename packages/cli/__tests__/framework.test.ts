import { resolveFramework } from '../src/framework';
import { verifyBinaryIntegrity } from '../src/framework';
import path from 'path';
import fs from 'fs';
import os from 'os';
import crypto from 'crypto';

describe('resolveFramework', () => {
  it('returns explicit path when provided and exists', async () => {
    const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'jig-test-'));
    const fwDir = path.join(tmpDir, 'Jig.framework');
    fs.mkdirSync(fwDir);
    fs.writeFileSync(path.join(fwDir, 'Jig'), 'fake-binary');

    const result = await resolveFramework(fwDir);
    expect(result).toBe(path.join(fwDir, 'Jig'));

    fs.rmSync(tmpDir, { recursive: true });
  });

  it('throws when explicit path does not exist', async () => {
    await expect(resolveFramework('/nonexistent/Jig.framework'))
      .rejects.toThrow('not found');
  });

  it('throws with helpful message when no framework found', async () => {
    try {
      await resolveFramework();
    } catch (err) {
      expect((err as Error).message).toMatch(/not found/i);
      expect((err as Error).message).toContain('build:framework');
    }
  });
});

describe('verifyBinaryIntegrity', () => {
  it('passes when hash matches manifest', async () => {
    const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'jig-test-'));
    const binaryPath = path.join(tmpDir, 'Jig');
    const content = 'fake-binary-content';
    fs.writeFileSync(binaryPath, content);

    const hash = crypto.createHash('sha256').update(content).digest('hex');
    const manifestPath = path.join(tmpDir, 'jig-binaries.sha256');
    fs.writeFileSync(manifestPath, `${hash}  Jig.framework/Jig\n`);

    await expect(verifyBinaryIntegrity(binaryPath, manifestPath)).resolves.not.toThrow();

    fs.rmSync(tmpDir, { recursive: true });
  });

  it('throws when hash does not match', async () => {
    const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'jig-test-'));
    const binaryPath = path.join(tmpDir, 'Jig');
    fs.writeFileSync(binaryPath, 'actual-content');

    const manifestPath = path.join(tmpDir, 'jig-binaries.sha256');
    fs.writeFileSync(manifestPath, 'badhash000  Jig.framework/Jig\n');

    await expect(verifyBinaryIntegrity(binaryPath, manifestPath))
      .rejects.toThrow('integrity');

    fs.rmSync(tmpDir, { recursive: true });
  });

  it('throws when manifest is missing', async () => {
    const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'jig-test-'));
    const binaryPath = path.join(tmpDir, 'Jig');
    fs.writeFileSync(binaryPath, 'content');

    await expect(verifyBinaryIntegrity(binaryPath, path.join(tmpDir, 'missing.sha256')))
      .rejects.toThrow('manifest');

    fs.rmSync(tmpDir, { recursive: true });
  });
});
