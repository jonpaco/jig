import type {
  ServerHelloMessage,
  ClientHelloMessage,
  SessionReadyMessage,
  HandshakeErrorMessage,
} from '../src';

describe('handshake message types', () => {
  it('ServerHelloMessage matches the design spec example', () => {
    const msg: ServerHelloMessage = {
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
    };
    expect(msg.jsonrpc).toBe('2.0');
    expect(msg.method).toBe('server.hello');
    expect(msg.params.protocol.name).toBe('jig');
    expect(msg.params.app.platform).toBe('ios');
  });

  it('ClientHelloMessage matches the design spec example', () => {
    const msg: ClientHelloMessage = {
      jsonrpc: '2.0',
      id: 1,
      method: 'client.hello',
      params: {
        protocol: { version: 1 },
        client: { name: '@jig/cli', version: '0.1.0' },
      },
    };
    expect(msg.id).toBe(1);
    expect(msg.method).toBe('client.hello');
    expect(msg.params.client.name).toBe('@jig/cli');
  });

  it('SessionReadyMessage matches the design spec example', () => {
    const msg: SessionReadyMessage = {
      jsonrpc: '2.0',
      id: 1,
      result: {
        sessionId: 'sess_a1b2c3',
        negotiated: { protocol: 1 },
        enabled: [],
        rejected: [],
      },
    };
    expect(msg.id).toBe(1);
    expect(msg.result.sessionId).toMatch(/^sess_/);
    expect(msg.result.negotiated.protocol).toBe(1);
  });

  it('HandshakeErrorMessage matches the design spec example', () => {
    const msg: HandshakeErrorMessage = {
      jsonrpc: '2.0',
      id: 1,
      error: {
        code: -32001,
        message: 'Protocol version 3 not supported. Server supports versions 1-2.',
        data: { supported: { min: 1, max: 2 } },
      },
    };
    expect(msg.error.code).toBe(-32001);
    expect(msg.error.data?.supported.min).toBe(1);
  });
});
