import type { Config } from 'jest';

const config: Config = {
  displayName: 'cli',
  testEnvironment: 'node',
  roots: ['<rootDir>'],
  testMatch: ['<rootDir>/__tests__/**/*.test.ts'],
  testPathIgnorePatterns: ['<rootDir>/__tests__/integration/'],
  transform: {
    '^.+\\.ts$': '@swc/jest',
  },
  moduleNameMapper: {
    '^@jig/protocol$': '<rootDir>/../protocol/src',
    '^@jig/sdk$': '<rootDir>/../sdk/src',
  },
};

export default config;
