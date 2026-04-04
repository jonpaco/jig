import { connect } from '../../src/client/connection';

/**
 * These tests verify screenshot capture against a real running app.
 * They require a running app with @jig/native on port 4042.
 *
 * Run: pnpm test -- --selectProjects sdk --testPathPattern screenshot.test
 */

describe('Screenshot Integration', () => {
  it('captures a PNG screenshot', async () => {
    const session = await connect({ port: 4042 });
    const result = await session.screenshot({ format: 'png' });

    expect(result.data).toBeTruthy();
    expect(result.data.length).toBeGreaterThan(100);
    expect(result.format).toBe('png');
    expect(result.width).toBeGreaterThan(0);
    expect(result.height).toBeGreaterThan(0);
    expect(result.timestamp).toBeGreaterThan(0);
    session.disconnect();
  });

  it('captures a JPEG screenshot with quality param', async () => {
    const session = await connect({ port: 4042 });
    const result = await session.screenshot({ format: 'jpeg', quality: 50 });

    expect(result.format).toBe('jpeg');
    expect(result.data).toBeTruthy();
    expect(result.width).toBeGreaterThan(0);
    session.disconnect();
  });
});
