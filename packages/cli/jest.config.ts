import type { Config } from 'jest';

const config: Config = {
  displayName: 'cli',
  testEnvironment: 'node',
  roots: ['<rootDir>'],
  testMatch: ['<rootDir>/__tests__/**/*.test.ts'],
  transform: {
    '^.+\\.ts$': '@swc/jest',
  },
  moduleNameMapper: {
    '^@jig/protocol$': '<rootDir>/../protocol/src',
  },
};

export default config;
