export class JigError extends Error {
  readonly code: number;
  readonly data?: unknown;

  constructor(code: number, message: string, data?: unknown) {
    super(message);
    this.name = 'JigError';
    this.code = code;
    this.data = data;
  }
}
