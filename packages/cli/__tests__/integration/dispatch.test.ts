// packages/cli/__tests__/integration/dispatch.test.ts

import WebSocket from 'ws';

const PORT = 4042;
const WS_URL = `ws://localhost:${PORT}`;

/**
 * These tests verify the dispatch architecture's wire-protocol behavior.
 * They require a running app with @jig/native on port 4042.
 *
 * Run: npx jest __tests__/integration/dispatch.test.ts
 */

function connect(): Promise<{ ws: WebSocket; serverHello: any }> {
  return new Promise((resolve, reject) => {
    const ws = new WebSocket(WS_URL);
    const timeout = setTimeout(() => {
      ws.close();
      reject(new Error('Connection timeout — is the app running?'));
    }, 5000);
    ws.once('message', (data) => {
      clearTimeout(timeout);
      resolve({ ws, serverHello: JSON.parse(data.toString()) });
    });
    ws.once('error', (err) => {
      clearTimeout(timeout);
      reject(err);
    });
  });
}

function sendAndReceive(ws: WebSocket, message: object): Promise<any> {
  return new Promise((resolve, reject) => {
    const timeout = setTimeout(() => reject(new Error('Response timeout')), 5000);
    ws.once('message', (data) => {
      clearTimeout(timeout);
      resolve(JSON.parse(data.toString()));
    });
    ws.send(JSON.stringify(message));
  });
}

function completeHandshake(ws: WebSocket): Promise<any> {
  return sendAndReceive(ws, {
    jsonrpc: '2.0',
    id: 1,
    method: 'client.hello',
    params: {
      protocol: { version: 1 },
      client: { name: 'dispatch-test', version: '0.0.0' },
    },
  });
}

describe('Dispatch Architecture', () => {
  afterEach(() => {
    // Give the server a moment between rapid connect/disconnect cycles
  });

  test('sends server.hello on connect', async () => {
    const { ws, serverHello } = await connect();
    try {
      expect(serverHello.jsonrpc).toBe('2.0');
      expect(serverHello.method).toBe('server.hello');
      expect(serverHello.params.protocol.version).toBe(1);
      expect(serverHello.params.protocol.name).toBe('jig');
      expect(serverHello.params.app).toBeDefined();
      expect(serverHello.params.app.platform).toMatch(/^(ios|android)$/);
    } finally {
      ws.close();
    }
  });

  test('rejects commands before handshake with HandshakeRequiredError (-32000)', async () => {
    const { ws } = await connect();
    try {
      const response = await sendAndReceive(ws, {
        jsonrpc: '2.0',
        id: 10,
        method: 'jig.screenshot',
      });
      expect(response.id).toBe(10);
      expect(response.error).toBeDefined();
      expect(response.error.code).toBe(-32000);
    } finally {
      ws.close();
    }
  });

  test('completes handshake successfully', async () => {
    const { ws } = await connect();
    try {
      const response = await completeHandshake(ws);
      expect(response.id).toBe(1);
      expect(response.result).toBeDefined();
      expect(response.result.sessionId).toMatch(/^sess_[a-f0-9]{8}$/i);
      expect(response.result.negotiated.protocol).toBe(1);
      expect(response.result.enabled).toEqual([]);
      expect(response.result.rejected).toEqual([]);
    } finally {
      ws.close();
    }
  });

  test('returns MethodNotFound (-32601) for unknown method after handshake', async () => {
    const { ws } = await connect();
    try {
      await completeHandshake(ws);
      const response = await sendAndReceive(ws, {
        jsonrpc: '2.0',
        id: 2,
        method: 'nonexistent.method',
      });
      expect(response.id).toBe(2);
      expect(response.error).toBeDefined();
      expect(response.error.code).toBe(-32601);
    } finally {
      ws.close();
    }
  });

  test('returns parse error (-32700) for malformed JSON', async () => {
    const { ws } = await connect();
    try {
      const response = await new Promise<any>((resolve, reject) => {
        const timeout = setTimeout(() => reject(new Error('Response timeout')), 5000);
        ws.once('message', (data) => {
          clearTimeout(timeout);
          resolve(JSON.parse(data.toString()));
        });
        ws.send('this is not json');
      });
      expect(response.error).toBeDefined();
      expect(response.error.code).toBe(-32700);
    } finally {
      ws.close();
    }
  });

  test('returns invalid request (-32600) for missing method field', async () => {
    const { ws } = await connect();
    try {
      const response = await sendAndReceive(ws, {
        jsonrpc: '2.0',
        id: 3,
      } as any);
      expect(response.error).toBeDefined();
      expect(response.error.code).toBe(-32600);
    } finally {
      ws.close();
    }
  });
});
