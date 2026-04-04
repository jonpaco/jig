import WebSocket from 'ws';
import type { ServerHelloParams, ScreenshotParams, ScreenshotResult } from '@jig/protocol';
import { JigError } from '../errors';

export interface ConnectOptions {
  host?: string;
  port?: number;
  timeout?: number;
  clientName?: string;
  clientVersion?: string;
}

export interface Session {
  readonly serverHello: ServerHelloParams;
  readonly sessionId: string;
  send<TResult = unknown>(method: string, params?: Record<string, unknown>): Promise<TResult>;
  screenshot(params?: ScreenshotParams): Promise<ScreenshotResult>;
  disconnect(): void;
}

export function connect(options: ConnectOptions = {}): Promise<Session> {
  const {
    host = 'localhost',
    port = 4042,
    timeout = 5000,
    clientName = '@jig/sdk',
    clientVersion = '0.1.0',
  } = options;
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
        ws.send(JSON.stringify({
          jsonrpc: '2.0',
          id: 1,
          method: 'client.hello',
          params: {
            protocol: { version: serverHello!.protocol.version },
            client: { name: clientName, version: clientVersion },
          },
        }));
        return;
      }

      if (msg.id === 1 && msg.result) {
        clearTimeout(timer);
        resolve(createSession(ws, serverHello!, msg.result.sessionId));
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

function createSession(
  ws: WebSocket,
  serverHello: ServerHelloParams,
  sessionId: string,
): Session {
  let nextId = 2; // 1 was used for client.hello
  const pending = new Map<number, {
    resolve: (result: any) => void;
    reject: (err: Error) => void;
    timer: ReturnType<typeof setTimeout>;
  }>();

  // Replace the handshake message handler with the command response router
  ws.removeAllListeners('message');
  ws.on('message', (raw: WebSocket.RawData) => {
    const msg = JSON.parse(raw.toString());
    if (msg.id != null && pending.has(msg.id)) {
      const entry = pending.get(msg.id)!;
      pending.delete(msg.id);
      clearTimeout(entry.timer);
      if (msg.error) {
        entry.reject(new JigError(msg.error.code, msg.error.message, msg.error.data));
      } else {
        entry.resolve(msg.result);
      }
    }
  });

  function send<TResult = unknown>(
    method: string,
    params?: Record<string, unknown>,
  ): Promise<TResult> {
    return new Promise((resolve, reject) => {
      if (ws.readyState !== WebSocket.OPEN) {
        reject(new Error('Not connected'));
        return;
      }
      const id = nextId++;
      const timer = setTimeout(() => {
        pending.delete(id);
        reject(new Error(`Command '${method}' timed out after 30000ms`));
      }, 30000);
      pending.set(id, { resolve, reject, timer });
      ws.send(JSON.stringify({ jsonrpc: '2.0', id, method, params: params ?? {} }));
    });
  }

  function disconnect(): void {
    for (const entry of pending.values()) {
      clearTimeout(entry.timer);
      entry.reject(new Error('Disconnected'));
    }
    pending.clear();
    ws.close();
  }

  function screenshot(params?: ScreenshotParams): Promise<ScreenshotResult> {
    const wireParams: Record<string, unknown> = {};
    if (params?.format) wireParams.format = params.format;
    if (params?.quality != null) wireParams.quality = params.quality;
    return send<ScreenshotResult>('jig.screenshot', wireParams);
  }

  return { serverHello, sessionId, send, screenshot, disconnect };
}
