import { WebSocketServer } from 'ws';
import { connect } from '../src/client/connection';

function startMockServer(opts?: {
  skipHello?: boolean;
  errorOnHandshake?: boolean;
}): Promise<{ server: WebSocketServer; port: number }> {
  return new Promise((resolve) => {
    const server = new WebSocketServer({ port: 0 }, () => {
      const port = (server.address() as { port: number }).port;

      server.on('connection', (ws) => {
        if (opts?.skipHello) return;

        ws.send(JSON.stringify({
          jsonrpc: '2.0',
          method: 'server.hello',
          params: {
            protocol: { name: 'jig', version: 1, min: 1, max: 1 },
            server: '0.1.0',
            app: {
              name: 'TestApp',
              bundleId: 'com.test.app',
              platform: 'ios',
              rn: '0.81.5',
            },
            commands: [],
            domains: [],
          },
        }));

        ws.on('message', (data) => {
          const msg = JSON.parse(data.toString());
          if (msg.method === 'client.hello') {
            if (opts?.errorOnHandshake) {
              ws.send(JSON.stringify({
                jsonrpc: '2.0',
                id: msg.id,
                error: {
                  code: -32001,
                  message: 'Protocol version not supported',
                  data: { supported: { min: 1, max: 1 } },
                },
              }));
            } else {
              ws.send(JSON.stringify({
                jsonrpc: '2.0',
                id: msg.id,
                result: {
                  sessionId: 'sess_test1234',
                  negotiated: { protocol: 1 },
                  enabled: [],
                  rejected: [],
                },
              }));
            }
          }
        });
      });

      resolve({ server, port });
    });
  });
}

describe('connect', () => {
  let server: WebSocketServer;
  let port: number;

  afterEach((done) => {
    if (server) {
      const s = server;
      server = undefined as unknown as WebSocketServer;
      s.close(() => done());
    } else {
      done();
    }
  });

  it('completes handshake and returns server info + session', async () => {
    ({ server, port } = await startMockServer());

    const result = await connect({ port });
    expect(result.serverHello.app.name).toBe('TestApp');
    expect(result.serverHello.app.bundleId).toBe('com.test.app');
    expect(result.serverHello.protocol.name).toBe('jig');
    expect(result.session.sessionId).toBe('sess_test1234');
    expect(result.session.negotiated.protocol).toBe(1);
  });

  it('rejects when server never sends hello', async () => {
    ({ server, port } = await startMockServer({ skipHello: true }));

    await expect(connect({ port, timeout: 200 }))
      .rejects.toThrow('timed out');
  });

  it('rejects on handshake error response', async () => {
    ({ server, port } = await startMockServer({ errorOnHandshake: true }));

    await expect(connect({ port }))
      .rejects.toThrow('Handshake failed');
  });

  it('rejects when no server is running', async () => {
    await expect(connect({ port: 19999, timeout: 200 }))
      .rejects.toThrow();
  });
});
