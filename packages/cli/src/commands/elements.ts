import { connect, type ConnectOptions, type Session } from '@jig/sdk';
import type { Selector, Element } from '@jig/protocol';

export interface ElementsOptions extends ConnectOptions {
  testID?: string;
  text?: string;
  role?: string;
  all?: boolean;
  json?: boolean;
}

export async function elements(options: ElementsOptions): Promise<string> {
  const session = await connect({
    host: options.host,
    port: options.port,
    timeout: options.timeout,
    clientName: '@jig/cli',
    clientVersion: options.clientVersion,
  });

  const selector: Selector = {};
  if (options.testID) selector.testID = options.testID;
  if (options.text) selector.text = options.text;
  if (options.role) selector.role = options.role;

  const result = await session.findElements(selector);
  session.disconnect();

  let matched = result.elements as Element[];
  if (!options.all) {
    matched = matched.filter((el) => el.visible);
  }

  if (options.json) {
    return JSON.stringify(matched, null, 2);
  }

  if (matched.length === 0) {
    return 'No elements found';
  }

  const rows = matched.map((el) => ({
    testID: el.testID ?? '',
    text: el.text ?? '',
    role: el.role ?? '',
    frame: `{${el.frame.x},${el.frame.y},${el.frame.width},${el.frame.height}}`,
    visible: String(el.visible),
  }));

  const cols = ['testID', 'text', 'role', 'frame', 'visible'] as const;
  const widths: Record<string, number> = {};
  for (const col of cols) {
    widths[col] = Math.max(col.length, ...rows.map((r) => r[col].length));
  }

  const header = cols.map((c) => c.padEnd(widths[c])).join('  ');
  const lines = rows.map((r) => cols.map((c) => r[c].padEnd(widths[c])).join('  '));

  return [
    `  ${header}`,
    ...lines.map((l) => `  ${l}`),
    '',
    `  ${matched.length} element${matched.length === 1 ? '' : 's'} found`,
  ].join('\n');
}
