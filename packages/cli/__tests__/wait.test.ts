import { WebSocketServer } from 'ws';
import { wait } from '../src/commands/wait';

function startMockServer(port = 0): Promise<{ server: WebSocketServer; port: number }> {
  return new Promise((resolve) => {
    const server = new WebSocketServer({ port }, () => {
      const actualPort = (server.address() as { port: number }).port;

      server.on('connection', (ws) => {
        ws.send(JSON.stringify({
          jsonrpc: '2.0',
          method: 'server.hello',
          params: {
            protocol: { name: 'jig', version: 1, min: 1, max: 1 },
            server: '0.1.0',
            app: {
              name: 'HabitTracker',
              bundleId: 'com.example.habittracker',
              platform: 'ios',
              rn: '0.81.5',
              expo: '54.0.23',
            },
            commands: [],
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
                sessionId: 'sess_abc12345',
                negotiated: { protocol: 1 },
                enabled: [],
                rejected: [],
              },
            }));
          }
        });
      });

      resolve({ server, port: actualPort });
    });
  });
}

describe('jig wait', () => {
  let server: WebSocketServer | undefined;

  afterEach((done) => {
    if (server) {
      const s = server;
      server = undefined;
      s.close(() => done());
    } else {
      done();
    }
  });

  it('resolves immediately when server is already running', async () => {
    ({ server } = await startMockServer());
    const port = (server.address() as { port: number }).port;
    const output = await wait({ port, timeout: 5000, interval: 100 });
    expect(output).toContain('Connected to HabitTracker');
  });

  it('retries until server becomes available', async () => {
    // Grab a free port
    const tmp = new WebSocketServer({ port: 0 });
    const port = (tmp.address() as { port: number }).port;
    await new Promise<void>((resolve) => tmp.close(() => resolve()));

    // Start mock server on that port after 300ms
    setTimeout(async () => {
      ({ server } = await startMockServer(port));
    }, 300);

    const output = await wait({ port, timeout: 5000, interval: 100 });
    expect(output).toContain('Connected');
  });

  it('times out when no server is available', async () => {
    await expect(
      wait({ port: 19999, timeout: 500, interval: 100 })
    ).rejects.toThrow('Timed out');
  });
});
