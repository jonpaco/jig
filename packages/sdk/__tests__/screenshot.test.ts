import { WebSocketServer, WebSocket as WsWebSocket } from 'ws';
import { connect } from '../src/client/connection';
import { JigError } from '../src/errors';

// Minimal 1x1 red PNG as base64 (68 bytes)
const TINY_PNG = 'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8/5+hHgAHggJ/PchI7wAAAABJRU5ErkJggg==';

function startScreenshotServer(opts?: {
  onScreenshot?: (params: any) => any;
}): Promise<{ server: WebSocketServer; port: number }> {
  return new Promise((resolve) => {
    const server = new WebSocketServer({ port: 0 }, () => {
      const port = (server.address() as { port: number }).port;

      server.on('connection', (ws) => {
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
            return;
          }

          if (msg.method === 'jig.screenshot') {
            if (opts?.onScreenshot) {
              const response = opts.onScreenshot(msg.params);
              ws.send(JSON.stringify({ jsonrpc: '2.0', id: msg.id, ...response }));
              return;
            }

            ws.send(JSON.stringify({
              jsonrpc: '2.0',
              id: msg.id,
              result: {
                data: TINY_PNG,
                format: msg.params?.format || 'png',
                width: 1,
                height: 1,
                timestamp: 1743638400000,
              },
            }));
          }
        });
      });

      resolve({ server, port });
    });
  });
}

describe('session.screenshot', () => {
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

  it('returns screenshot result with default params', async () => {
    ({ server, port } = await startScreenshotServer());

    const session = await connect({ port });
    const result = await session.screenshot();

    expect(result.data).toBe(TINY_PNG);
    expect(result.format).toBe('png');
    expect(result.width).toBe(1);
    expect(result.height).toBe(1);
    expect(result.timestamp).toBe(1743638400000);
    session.disconnect();
  });

  it('passes format and quality params', async () => {
    let receivedParams: any = null;
    ({ server, port } = await startScreenshotServer({
      onScreenshot: (params) => {
        receivedParams = params;
        return {
          result: {
            data: TINY_PNG,
            format: 'jpeg',
            width: 1,
            height: 1,
            timestamp: 1743638400000,
          },
        };
      },
    }));

    const session = await connect({ port });
    const result = await session.screenshot({ format: 'jpeg', quality: 50 });

    expect(receivedParams.format).toBe('jpeg');
    expect(receivedParams.quality).toBe(50);
    expect(result.format).toBe('jpeg');
    session.disconnect();
  });

  it('throws JigError when server returns error', async () => {
    ({ server, port } = await startScreenshotServer({
      onScreenshot: () => ({
        error: { code: -32603, message: 'No active window available for screenshot' },
      }),
    }));

    const session = await connect({ port });
    try {
      await session.screenshot();
      fail('should have thrown');
    } catch (err) {
      expect(err).toBeInstanceOf(JigError);
      expect((err as JigError).code).toBe(-32603);
      expect((err as JigError).message).toContain('No active window');
    }
    session.disconnect();
  });
});
