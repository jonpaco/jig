/** JSON-RPC 2.0 notification (no id, no response expected) */
export interface JsonRpcNotification<TParams> {
  jsonrpc: '2.0';
  method: string;
  params: TParams;
}

/** JSON-RPC 2.0 request (has id, expects a response) */
export interface JsonRpcRequest<TParams> {
  jsonrpc: '2.0';
  id: number;
  method: string;
  params: TParams;
}

/** JSON-RPC 2.0 success response */
export interface JsonRpcResponse<TResult> {
  jsonrpc: '2.0';
  id: number;
  result: TResult;
}

/** JSON-RPC 2.0 error response */
export interface JsonRpcErrorResponse<TData = unknown> {
  jsonrpc: '2.0';
  id: number;
  error: {
    code: number;
    message: string;
    data?: TData;
  };
}
