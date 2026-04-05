import type { Config } from 'jest';

const config: Config = {
  displayName: 'device',
  testEnvironment: 'node',
  transform: {
    '^.+\\.ts$': ['@swc/jest', {}],
  },
  testMatch: ['<rootDir>/__tests__/**/*.test.ts'],
};

export default config;
