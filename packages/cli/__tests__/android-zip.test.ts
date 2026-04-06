import * as yazl from 'yazl';
import * as fs from 'fs';
import * as path from 'path';
import * as os from 'os';
import { listEntries, extractEntry, addEntries, replaceEntry } from '../src/android/zip';

/** Build a simple ZIP in memory and write it to disk. */
function createTestZip(
  destPath: string,
  entries: Record<string, Buffer>,
): Promise<void> {
  return new Promise((resolve, reject) => {
    const zip = new yazl.ZipFile();
    const out = fs.createWriteStream(destPath);
    zip.outputStream.pipe(out);
    out.on('finish', resolve);
    out.on('error', reject);
    zip.outputStream.on('error', reject);
    for (const [name, buf] of Object.entries(entries)) {
      zip.addBuffer(buf, name);
    }
    zip.end();
  });
}

describe('listEntries', () => {
  let tmpDir: string;

  beforeEach(() => {
    tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'jig-zip-'));
  });

  afterEach(() => {
    fs.rmSync(tmpDir, { recursive: true });
  });

  it('lists all entries in a ZIP', async () => {
    const zipPath = path.join(tmpDir, 'test.zip');
    await createTestZip(zipPath, {
      'a.txt': Buffer.from('alpha'),
      'b.txt': Buffer.from('bravo'),
      'lib/libjig.so': Buffer.from('so-data'),
    });

    const entries = await listEntries(zipPath);
    expect(entries).toHaveLength(3);
    expect(entries).toContain('a.txt');
    expect(entries).toContain('b.txt');
    expect(entries).toContain('lib/libjig.so');
  });
});

describe('addEntries', () => {
  let tmpDir: string;

  beforeEach(() => {
    tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'jig-zip-'));
  });

  afterEach(() => {
    fs.rmSync(tmpDir, { recursive: true });
  });

  it('adds new entries without modifying existing ones', async () => {
    const inputPath = path.join(tmpDir, 'input.zip');
    const outputPath = path.join(tmpDir, 'output.zip');

    await createTestZip(inputPath, {
      'existing.txt': Buffer.from('original content'),
    });

    await addEntries(inputPath, outputPath, {
      'new.txt': Buffer.from('new content'),
      'lib/libjig.so': Buffer.from('so-bytes'),
    });

    const entries = await listEntries(outputPath);
    expect(entries).toHaveLength(3);
    expect(entries).toContain('existing.txt');
    expect(entries).toContain('new.txt');
    expect(entries).toContain('lib/libjig.so');

    // Verify existing entry content is unchanged
    const existingContent = await extractEntry(outputPath, 'existing.txt');
    expect(existingContent.toString()).toBe('original content');

    // Verify new entry content is correct
    const newContent = await extractEntry(outputPath, 'new.txt');
    expect(newContent.toString()).toBe('new content');

    const soContent = await extractEntry(outputPath, 'lib/libjig.so');
    expect(soContent.toString()).toBe('so-bytes');
  });
});

describe('replaceEntry', () => {
  let tmpDir: string;

  beforeEach(() => {
    tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'jig-zip-'));
  });

  afterEach(() => {
    fs.rmSync(tmpDir, { recursive: true });
  });

  it('replaces an existing entry with new content', async () => {
    const inputPath = path.join(tmpDir, 'input.zip');
    const outputPath = path.join(tmpDir, 'output.zip');

    await createTestZip(inputPath, {
      'classes.dex': Buffer.from('original dex'),
      'other.txt': Buffer.from('keep me'),
    });

    await replaceEntry(inputPath, outputPath, 'classes.dex', Buffer.from('patched dex'));

    const entries = await listEntries(outputPath);
    expect(entries).toHaveLength(2);
    expect(entries).toContain('classes.dex');
    expect(entries).toContain('other.txt');

    const replaced = await extractEntry(outputPath, 'classes.dex');
    expect(replaced.toString()).toBe('patched dex');

    // Other entry is unchanged
    const kept = await extractEntry(outputPath, 'other.txt');
    expect(kept.toString()).toBe('keep me');
  });

  it('throws when the target entry does not exist', async () => {
    const inputPath = path.join(tmpDir, 'input.zip');
    const outputPath = path.join(tmpDir, 'output.zip');

    await createTestZip(inputPath, {
      'classes.dex': Buffer.from('some dex'),
    });

    await expect(
      replaceEntry(inputPath, outputPath, 'nonexistent.txt', Buffer.from('data')),
    ).rejects.toThrow('Entry not found: nonexistent.txt');
  });
});
