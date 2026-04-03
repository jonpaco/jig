import { execSync } from 'child_process';
import { readFileSync, existsSync } from 'fs';
import { join } from 'path';

const ROOT = join(__dirname, '..');

describe('codegen', () => {
  beforeAll(() => {
    execSync('pnpm codegen', { cwd: ROOT, stdio: 'pipe' });
  });

  it('generates an index that re-exports all schema files', () => {
    const indexPath = join(ROOT, 'src', 'generated', 'index.ts');
    expect(existsSync(indexPath)).toBe(true);
    const content = readFileSync(indexPath, 'utf-8');
    expect(content).toContain('server-hello-params');
    expect(content).toContain('client-hello-params');
    expect(content).toContain('session-ready-result');
    expect(content).toContain('handshake-error-data');
  });

  it('generates ServerHelloParams interface', () => {
    const filePath = join(ROOT, 'src', 'generated', 'server-hello-params.ts');
    const content = readFileSync(filePath, 'utf-8');
    expect(content).toContain('export interface ServerHelloParams');
  });

  it('generates ClientHelloParams interface', () => {
    const filePath = join(ROOT, 'src', 'generated', 'client-hello-params.ts');
    const content = readFileSync(filePath, 'utf-8');
    expect(content).toContain('export interface ClientHelloParams');
  });

  it('generates SessionReadyResult interface', () => {
    const filePath = join(ROOT, 'src', 'generated', 'session-ready-result.ts');
    const content = readFileSync(filePath, 'utf-8');
    expect(content).toContain('export interface SessionReadyResult');
  });

  it('generates HandshakeErrorData interface', () => {
    const filePath = join(ROOT, 'src', 'generated', 'handshake-error-data.ts');
    const content = readFileSync(filePath, 'utf-8');
    expect(content).toContain('export interface HandshakeErrorData');
  });
});
