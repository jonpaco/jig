import type { Config } from 'jest';

const config: Config = {
  projects: [
    '<rootDir>/packages/protocol',
    '<rootDir>/packages/cli',
    '<rootDir>/packages/sdk',
  ],
};

export default config;
