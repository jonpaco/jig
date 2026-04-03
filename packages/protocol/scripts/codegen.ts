import { compileFromFile } from 'json-schema-to-typescript';
import { readdir, writeFile, mkdir } from 'fs/promises';
import { join, basename } from 'path';

const SCHEMAS_DIR = join(__dirname, '..', 'schemas');
const OUTPUT_DIR = join(__dirname, '..', 'src', 'generated');

async function main() {
  await mkdir(OUTPUT_DIR, { recursive: true });

  const files = (await readdir(SCHEMAS_DIR))
    .filter(f => f.endsWith('.json'))
    .sort();

  const exports: string[] = [];

  for (const file of files) {
    const name = basename(file, '.json');
    const ts = await compileFromFile(join(SCHEMAS_DIR, file), {
      additionalProperties: false,
      bannerComment: `/* Auto-generated from schemas/${file} — do not edit */`,
      format: false,
    });
    await writeFile(join(OUTPUT_DIR, `${name}.ts`), ts + '\n');
    exports.push(`export * from './${name}';`);
  }

  const indexContent = '/* Auto-generated index — do not edit */\n' + exports.join('\n') + '\n';
  await writeFile(join(OUTPUT_DIR, 'index.ts'), indexContent);

  console.log(`Generated ${files.length} type files in src/generated/`);
}

main().catch(err => {
  console.error('Codegen failed:', err);
  process.exit(1);
});
