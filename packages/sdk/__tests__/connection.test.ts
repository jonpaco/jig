import { WebSocketServer, WebSocket as WsWebSocket } from 'ws';
import { connect } from '../src/client/connection';
import { JigError } from '../src/errors';

function startMockServer(opts?: {
  skipHello?: boolean;
  errorOnHandshake?: boolean;
  onCommand?: (method: string, params: any, id: number, ws: WsWebSocket) => void;
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
            commands: ['jig.screenshot'],
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
            return;
          }

          if (opts?.onCommand) {
            opts.onCommand(msg.method, msg.params, msg.id, ws);
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

  it('completes handshake and returns session with server info', async () => {
    ({ server, port } = await startMockServer());

    const session = await connect({ port });
    expect(session.serverHello.app.name).toBe('TestApp');
    expect(session.serverHello.protocol.name).toBe('jig');
    expect(session.sessionId).toBe('sess_test1234');
    session.disconnect();
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

  it('can send commands after handshake', async () => {
    ({ server, port } = await startMockServer({
      onCommand: (method, params, id, ws) => {
        ws.send(JSON.stringify({
          jsonrpc: '2.0',
          id,
          result: { echo: method },
        }));
      },
    }));

    const session = await connect({ port });
    const result = await session.send<{ echo: string }>('test.echo', {});
    expect(result.echo).toBe('test.echo');
    session.disconnect();
  });

  it('throws JigError on command error response', async () => {
    ({ server, port } = await startMockServer({
      onCommand: (_method, _params, id, ws) => {
        ws.send(JSON.stringify({
          jsonrpc: '2.0',
          id,
          error: { code: -32603, message: 'Something broke', data: { detail: 'oops' } },
        }));
      },
    }));

    const session = await connect({ port });
    try {
      await session.send('test.fail', {});
      fail('should have thrown');
    } catch (err) {
      expect(err).toBeInstanceOf(JigError);
      expect((err as JigError).code).toBe(-32603);
      expect((err as JigError).message).toBe('Something broke');
      expect((err as JigError).data).toEqual({ detail: 'oops' });
    }
    session.disconnect();
  });

  it('rejects pending commands on disconnect', async () => {
    ({ server, port } = await startMockServer({
      onCommand: () => {
        // Never respond — simulate hanging command
      },
    }));

    const session = await connect({ port });
    const pending = session.send('test.hang', {});
    session.disconnect();
    await expect(pending).rejects.toThrow('Disconnected');
  });
});
