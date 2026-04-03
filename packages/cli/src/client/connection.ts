import WebSocket from 'ws';
import type { ServerHelloParams, SessionReadyResult } from '@jig/protocol';

export interface ConnectionOptions {
  host?: string;
  port?: number;
  timeout?: number;
}

export interface ConnectionResult {
  serverHello: ServerHelloParams;
  session: SessionReadyResult;
}

export function connect(options: ConnectionOptions = {}): Promise<ConnectionResult> {
  const { host = 'localhost', port = 4042, timeout = 5000 } = options;
  const url = `ws://${host}:${port}`;

  return new Promise((resolve, reject) => {
    const ws = new WebSocket(url);
    const timer = setTimeout(() => {
      ws.close();
      reject(new Error(`Connection to ${url} timed out after ${timeout}ms`));
    }, timeout);

    let serverHello: ServerHelloParams | null = null;

    ws.on('error', (err) => {
      clearTimeout(timer);
      reject(new Error(`Failed to connect to ${url}: ${err.message}`));
    });

    ws.on('message', (raw: WebSocket.RawData) => {
      const msg = JSON.parse(raw.toString());

      if (msg.method === 'server.hello') {
        serverHello = msg.params;

        const clientHello = {
          jsonrpc: '2.0',
          id: 1,
          method: 'client.hello',
          params: {
            protocol: { version: serverHello!.protocol.version },
            client: {
              name: '@jig/cli',
              version: '0.1.0',
            },
          },
        };
        ws.send(JSON.stringify(clientHello));
        return;
      }

      if (msg.id === 1 && msg.result) {
        clearTimeout(timer);
        ws.close();
        resolve({ serverHello: serverHello!, session: msg.result });
        return;
      }

      if (msg.id === 1 && msg.error) {
        clearTimeout(timer);
        ws.close();
        reject(new Error(`Handshake failed: ${msg.error.message}`));
      }
    });
  });
}
