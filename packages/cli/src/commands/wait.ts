import { status } from './status';

export interface WaitOptions {
  host?: string;
  port?: number;
  timeout?: number;
  interval?: number;
}

export async function wait(options: WaitOptions = {}): Promise<string> {
  const {
    host = 'localhost',
    port = 4042,
    timeout = 30000,
    interval = 1000,
  } = options;

  const deadline = Date.now() + timeout;

  while (Date.now() < deadline) {
    try {
      const output = await status({
        host,
        port,
        timeout: Math.min(interval, deadline - Date.now()),
        clientName: '@jig/cli',
        clientVersion: '0.1.0',
      });
      return output;
    } catch {
      const remaining = deadline - Date.now();
      if (remaining <= 0) break;
      await new Promise((resolve) => setTimeout(resolve, Math.min(interval, remaining)));
    }
  }

  throw new Error(`Timed out waiting for Jig server at ws://${host}:${port} after ${timeout}ms`);
}
