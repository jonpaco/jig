package expo.modules.jig;

import android.os.Handler;
import android.os.Looper;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class JigDispatcher {
    private static final int PROTOCOL_VERSION = 1;
    private static final String SERVER_VERSION = "0.1.0";

    private final List<JigMiddleware> middlewares;
    private final Map<String, JigHandler> handlerMap;
    private final List<String> supportedDomains;
    private final List<String> commandNames;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());

    public JigDispatcher(List<JigMiddleware> middlewares,
                         List<JigHandler> handlers,
                         List<String> supportedDomains) {
        this.middlewares = middlewares;
        this.supportedDomains = supportedDomains;

        this.handlerMap = new HashMap<>();
        this.commandNames = new ArrayList<>();
        for (JigHandler handler : handlers) {
            String method = handler.getMethod();
            if (handlerMap.containsKey(method)) {
                throw new IllegalArgumentException(
                    "Duplicate handler for method: " + method);
            }
            handlerMap.put(method, handler);
            if (!"client.hello".equals(method)) {
                commandNames.add(method);
            }
        }
    }

    /** Send server.hello to a newly connected client */
    public void handleOpen(JigSessionContext context) {
        try {
            JSONObject protocol = new JSONObject();
            protocol.put("name", "jig");
            protocol.put("version", PROTOCOL_VERSION);
            protocol.put("min", PROTOCOL_VERSION);
            protocol.put("max", PROTOCOL_VERSION);

            JSONObject params = new JSONObject();
            params.put("protocol", protocol);
            params.put("server", SERVER_VERSION);
            params.put("app", context.getAppInfo().toJSON());
            params.put("commands", new JSONArray(commandNames));
            params.put("domains", new JSONArray(supportedDomains));

            JSONObject message = new JSONObject();
            message.put("jsonrpc", "2.0");
            message.put("method", "server.hello");
            message.put("params", params);

            context.sendText(message.toString());
        } catch (JSONException e) {
            System.out.println("[Jig] Failed to build server.hello: "
                               + e.getMessage());
        }
    }

    /** Parse, validate, route, and respond to an incoming message */
    public void dispatch(String text, JigSessionContext context) {
        // 1. Parse JSON
        JSONObject json;
        try {
            json = new JSONObject(text);
        } catch (JSONException e) {
            sendParseError("Malformed JSON", context);
            return;
        }

        // 2. Extract method, id, params
        String jsonrpc = json.optString("jsonrpc", "");
        String method = json.has("method") ? json.optString("method") : null;
        if (!"2.0".equals(jsonrpc) || method == null) {
            Integer id = json.has("id") ? json.optInt("id") : null;
            sendError(JigErrors.INVALID_REQUEST,
                "Missing or invalid 'jsonrpc' or 'method' field",
                null, id, context);
            return;
        }

        final Integer msgId = json.has("id") ? json.optInt("id") : null;
        final JSONObject params = json.optJSONObject("params");
        final boolean isCommand = (msgId != null);

        // 3. Run middleware chain
        for (JigMiddleware middleware : middlewares) {
            try {
                middleware.validate(method, params, context);
            } catch (JigException e) {
                if (isCommand) {
                    sendError(e.getCode(), e.getMessage(),
                              e.getData(), msgId, context);
                } else {
                    System.out.println("[Jig] Middleware rejected notification '"
                                       + method + "': " + e.getMessage());
                }
                return;
            }
        }

        // 4. Look up handler
        JigHandler handler = handlerMap.get(method);
        if (handler == null) {
            if (isCommand) {
                sendError(JigErrors.METHOD_NOT_FOUND,
                    "Method '" + method + "' not found",
                    null, msgId, context);
            } else {
                System.out.println("[Jig] No handler for notification '"
                                   + method + "'");
            }
            return;
        }

        // 5. Dispatch to handler's declared thread
        final String finalMethod = method;
        Runnable execute = () -> {
            executeHandler(handler, params, context,
                           finalMethod, msgId, isCommand);
        };

        switch (handler.getThreadTarget()) {
            case WEBSOCKET:
                execute.run();
                break;
            case MAIN:
                mainHandler.post(execute);
                break;
        }
    }

    // --- Handler execution ---

    private void executeHandler(JigHandler handler, JSONObject params,
                                JigSessionContext context, String method,
                                Integer msgId, boolean isCommand) {
        try {
            Object result = handler.handle(params, context);

            // Special case: update session context after successful handshake
            if ("client.hello".equals(method) && result instanceof JSONObject) {
                JSONObject resultObj = (JSONObject) result;
                context.setSessionId(resultObj.optString("sessionId"));
                JSONObject negotiated = resultObj.optJSONObject("negotiated");
                if (negotiated != null) {
                    context.setNegotiatedVersion(
                        negotiated.optInt("protocol"));
                }
            }

            if (isCommand) {
                sendResult(result, msgId, context);
            }
        } catch (JigException e) {
            if (isCommand) {
                sendError(e.getCode(), e.getMessage(),
                          e.getData(), msgId, context);
            } else {
                System.out.println("[Jig] Handler error for '" + method
                                   + "': " + e.getMessage());
            }
        } catch (Exception e) {
            if (isCommand) {
                sendError(JigErrors.INTERNAL_ERROR,
                    "Internal error: " + e.getMessage(),
                    null, msgId, context);
            } else {
                System.out.println("[Jig] Internal error for '" + method
                                   + "': " + e.getMessage());
            }
        }
    }

    // --- Response formatting ---

    private void sendResult(Object result, int msgId,
                            JigSessionContext context) {
        try {
            JSONObject response = new JSONObject();
            response.put("jsonrpc", "2.0");
            response.put("id", msgId);
            response.put("result", result != null ? result : JSONObject.NULL);
            context.sendText(response.toString());
        } catch (JSONException e) {
            System.out.println("[Jig] Failed to format result: "
                               + e.getMessage());
        }
    }

    private void sendError(int code, String message, JSONObject data,
                           Integer msgId, JigSessionContext context) {
        try {
            JSONObject error = new JSONObject();
            error.put("code", code);
            error.put("message", message);
            if (data != null) {
                error.put("data", data);
            }

            JSONObject response = new JSONObject();
            response.put("jsonrpc", "2.0");
            response.put("id", msgId != null ? (Object) msgId : JSONObject.NULL);
            response.put("error", error);
            context.sendText(response.toString());
        } catch (JSONException e) {
            System.out.println("[Jig] Failed to format error: "
                               + e.getMessage());
        }
    }

    private void sendParseError(String message, JigSessionContext context) {
        try {
            JSONObject error = new JSONObject();
            error.put("code", JigErrors.PARSE_ERROR);
            error.put("message", message);

            JSONObject response = new JSONObject();
            response.put("jsonrpc", "2.0");
            response.put("id", JSONObject.NULL);
            response.put("error", error);
            context.sendText(response.toString());
        } catch (JSONException e) {
            System.out.println("[Jig] Failed to format parse error: "
                               + e.getMessage());
        }
    }
}
