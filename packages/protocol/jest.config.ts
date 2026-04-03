import type { Config } from 'jest';

const config: Config = {
  displayName: 'protocol',
  testEnvironment: 'node',
  roots: ['<rootDir>'],
  testMatch: ['<rootDir>/__tests__/**/*.test.ts'],
  transform: {
    '^.+\\.ts$': '@swc/jest',
  },
};

export default config;
