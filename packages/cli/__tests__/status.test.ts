import { WebSocketServer } from 'ws';
import { status } from '../src/commands/status';

function startMockServer(): Promise<{ server: WebSocketServer; port: number }> {
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

      resolve({ server, port });
    });
  });
}

describe('jig status', () => {
  let server: WebSocketServer;
  let port: number;

  beforeEach(async () => {
    ({ server, port } = await startMockServer());
  });

  afterEach((done) => {
    if (server) {
      const s = server;
      server = undefined as unknown as WebSocketServer;
      s.close(() => done());
    } else {
      done();
    }
  });

  it('prints formatted connection info', async () => {
    const output = await status({ port });
    expect(output).toContain('Connected to HabitTracker');
    expect(output).toContain('com.example.habittracker');
    expect(output).toContain('ios');
    expect(output).toContain('0.81.5');
    expect(output).toContain('54.0.23');
    expect(output).toContain('jig/1');
    expect(output).toContain('0.1.0');
  });

  it('fails gracefully when no server is running', async () => {
    const s = server;
    server = undefined as unknown as WebSocketServer;
    await new Promise<void>((resolve) => s.close(() => resolve()));
    await expect(status({ port })).rejects.toThrow();
  });
});
