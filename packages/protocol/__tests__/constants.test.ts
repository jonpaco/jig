import { DEFAULT_PORT, DEFAULT_HOST, DEFAULT_CONNECT_TIMEOUT, DEFAULT_WAIT_TIMEOUT, DEFAULT_RETRY_INTERVAL, CLIENT_VERSION } from '@jig/protocol';

describe('protocol constants', () => {
  it('exports expected default values', () => {
    expect(DEFAULT_PORT).toBe(4042);
    expect(DEFAULT_HOST).toBe('localhost');
    expect(DEFAULT_CONNECT_TIMEOUT).toBe(5000);
    expect(DEFAULT_WAIT_TIMEOUT).toBe(30000);
    expect(DEFAULT_RETRY_INTERVAL).toBe(1000);
    expect(typeof CLIENT_VERSION).toBe('string');
  });
});
