import { connect } from '@jig/sdk';
import { writeFileSync } from 'fs';

async function main() {
  console.log('Connecting to app...');
  const session = await connect();

  console.log(`Connected to ${session.serverHello.app.name} (${session.sessionId})`);

  console.log('Taking screenshot...');
  const result = await session.screenshot({ format: 'png' });

  const filename = 'screenshot.png';
  writeFileSync(filename, Buffer.from(result.data, 'base64'));
  console.log(`Saved ${filename} (${result.width}x${result.height}, ${result.format})`);

  session.disconnect();
}

main().catch((err) => {
  console.error('Error:', err.message);
  process.exit(1);
});
