import { status } from './status';
import { DEFAULT_HOST, DEFAULT_PORT, DEFAULT_WAIT_TIMEOUT, DEFAULT_RETRY_INTERVAL, CLIENT_VERSION } from '@jig/protocol';

export interface WaitOptions {
  host?: string;
  port?: number;
  timeout?: number;
  interval?: number;
}

export async function wait(options: WaitOptions = {}): Promise<string> {
  const {
    host = DEFAULT_HOST,
    port = DEFAULT_PORT,
    timeout = DEFAULT_WAIT_TIMEOUT,
    interval = DEFAULT_RETRY_INTERVAL,
  } = options;

  const deadline = Date.now() + timeout;

  while (Date.now() < deadline) {
    try {
      const output = await status({
        host,
        port,
        timeout: Math.min(interval, deadline - Date.now()),
        clientName: '@jig/cli',
        clientVersion: CLIENT_VERSION,
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
