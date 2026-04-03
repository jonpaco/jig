import type { JsonRpcNotification, JsonRpcRequest, JsonRpcResponse, JsonRpcErrorResponse } from './json-rpc';
import type { ServerHelloParams, ClientHelloParams, SessionReadyResult, HandshakeErrorData } from './generated';

export type ServerHelloMessage = JsonRpcNotification<ServerHelloParams> & { method: 'server.hello' };
export type ClientHelloMessage = JsonRpcRequest<ClientHelloParams> & { method: 'client.hello' };
export type SessionReadyMessage = JsonRpcResponse<SessionReadyResult>;
export type HandshakeErrorMessage = JsonRpcErrorResponse<HandshakeErrorData>;
