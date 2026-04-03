import JigNativeModule from './JigNativeModule';

export async function start(config?: { port?: number }): Promise<void> {
  let rnVersion = 'unknown';
  try {
    const v = require('react-native/Libraries/Core/ReactNativeVersion').version;
    rnVersion = `${v.major}.${v.minor}.${v.patch}`;
  } catch {}

  let expoVersion: string | undefined;
  try {
    expoVersion = require('expo/package.json').version;
  } catch {}

  await JigNativeModule.start({
    rnVersion,
    expoVersion,
    port: config?.port ?? 4042,
  });
}

export function stop(): void {
  JigNativeModule.stop();
}

export function isRunning(): boolean {
  return JigNativeModule.isRunning();
}
