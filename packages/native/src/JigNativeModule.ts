import { requireNativeModule } from 'expo-modules-core';

interface JigNativeInterface {
  start(config: {
    rnVersion: string;
    expoVersion?: string;
    port?: number;
  }): Promise<void>;
  stop(): void;
  isRunning(): boolean;
}

export default requireNativeModule<JigNativeInterface>('Jig');
