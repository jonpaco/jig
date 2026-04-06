import * as yauzl from 'yauzl';
import * as yazl from 'yazl';
import fs from 'fs';

/** Returns true if the entry should be stored without compression (e.g. native libraries). */
function shouldStore(entryName: string): boolean {
  return entryName.endsWith('.so');
}

/** Collect all entry names from a ZIP file. */
export function listEntries(zipPath: string): Promise<string[]> {
  return new Promise((resolve, reject) => {
    yauzl.open(zipPath, { lazyEntries: true, autoClose: true }, (err, zipfile) => {
      if (err) return reject(err);
      const names: string[] = [];
      zipfile.readEntry();
      zipfile.on('entry', (entry: yauzl.Entry) => {
        names.push(entry.fileName);
        zipfile.readEntry();
      });
      zipfile.on('end', () => resolve(names));
      zipfile.on('error', reject);
    });
  });
}

/** Extract a single entry from a ZIP file as a Buffer. */
export function extractEntry(zipPath: string, entryName: string): Promise<Buffer> {
  return new Promise((resolve, reject) => {
    yauzl.open(zipPath, { lazyEntries: true, autoClose: true }, (err, zipfile) => {
      if (err) return reject(err);
      let found = false;
      zipfile.readEntry();
      zipfile.on('entry', (entry: yauzl.Entry) => {
        if (entry.fileName !== entryName) {
          zipfile.readEntry();
          return;
        }
        found = true;
        zipfile.openReadStream(entry, (streamErr, readStream) => {
          if (streamErr) return reject(streamErr);
          const chunks: Buffer[] = [];
          readStream.on('data', (chunk: Buffer) => chunks.push(chunk));
          readStream.on('end', () => resolve(Buffer.concat(chunks)));
          readStream.on('error', reject);
        });
      });
      zipfile.on('end', () => {
        if (!found) reject(new Error(`Entry not found: ${entryName}`));
      });
      zipfile.on('error', reject);
    });
  });
}

/** Read all entries from a ZIP as an ordered list of { name, data } pairs. */
function readAllEntries(zipPath: string): Promise<Array<{ name: string; data: Buffer }>> {
  return new Promise((resolve, reject) => {
    yauzl.open(zipPath, { lazyEntries: true, autoClose: true }, (err, zipfile) => {
      if (err) return reject(err);

      const results: Array<{ name: string; data: Buffer }> = [];

      const readNext = () => zipfile.readEntry();

      zipfile.on('entry', (entry: yauzl.Entry) => {
        zipfile.openReadStream(entry, (streamErr, readStream) => {
          if (streamErr) return reject(streamErr);
          const chunks: Buffer[] = [];
          readStream.on('data', (chunk: Buffer) => chunks.push(chunk));
          readStream.on('end', () => {
            results.push({ name: entry.fileName, data: Buffer.concat(chunks) });
            readNext();
          });
          readStream.on('error', reject);
        });
      });

      zipfile.on('end', () => resolve(results));
      zipfile.on('error', reject);

      readNext();
    });
  });
}

/** Write an ordered list of entries to a ZIP file at outputPath. */
function writeZip(
  outputPath: string,
  entries: Array<{ name: string; data: Buffer }>,
): Promise<void> {
  return new Promise((resolve, reject) => {
    const zip = new yazl.ZipFile();
    const out = fs.createWriteStream(outputPath);
    zip.outputStream.pipe(out);
    out.on('finish', resolve);
    out.on('error', reject);
    zip.outputStream.on('error', reject);

    for (const { name, data } of entries) {
      zip.addBuffer(data, name, { compress: !shouldStore(name) });
    }
    zip.end();
  });
}

/**
 * Copy a ZIP file to outputPath, adding new entries from newEntries.
 * Existing entries are preserved unchanged. .so files are stored uncompressed.
 */
export async function addEntries(
  inputPath: string,
  outputPath: string,
  newEntries: Record<string, Buffer>,
): Promise<void> {
  const existing = await readAllEntries(inputPath);
  const added = Object.entries(newEntries).map(([name, data]) => ({ name, data }));
  await writeZip(outputPath, [...existing, ...added]);
}

/**
 * Copy a ZIP file to outputPath, replacing the content of one entry.
 * Throws if the entry is not found. .so files are stored uncompressed.
 */
export async function replaceEntry(
  inputPath: string,
  outputPath: string,
  entryName: string,
  newContent: Buffer,
): Promise<void> {
  const existing = await readAllEntries(inputPath);
  const found = existing.some(e => e.name === entryName);
  if (!found) {
    throw new Error(`Entry not found: ${entryName}`);
  }
  const updated = existing.map(e =>
    e.name === entryName ? { name: e.name, data: newContent } : e,
  );
  await writeZip(outputPath, updated);
}
