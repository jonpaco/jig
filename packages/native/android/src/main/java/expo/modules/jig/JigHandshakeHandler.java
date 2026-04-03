package expo.modules.jig;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.UUID;

public class JigHandshakeHandler {
    public static final int PROTOCOL_VERSION = 1;
    public static final String SERVER_VERSION = "0.1.0";

    private final String appName;
    private final String packageName;
    private final String rnVersion;
    private final String expoVersion; // nullable

    public JigHandshakeHandler(String appName, String packageName,
                               String rnVersion, String expoVersion) {
        this.appName = appName;
        this.packageName = packageName;
        this.rnVersion = rnVersion;
        this.expoVersion = expoVersion;
    }

    public String makeServerHello() {
        try {
            JSONObject app = new JSONObject();
            app.put("name", appName);
            app.put("bundleId", packageName);
            app.put("platform", "android");
            app.put("rn", rnVersion);
            if (expoVersion != null) {
                app.put("expo", expoVersion);
            }

            JSONObject protocol = new JSONObject();
            protocol.put("name", "jig");
            protocol.put("version", PROTOCOL_VERSION);
            protocol.put("min", PROTOCOL_VERSION);
            protocol.put("max", PROTOCOL_VERSION);

            JSONObject params = new JSONObject();
            params.put("protocol", protocol);
            params.put("server", SERVER_VERSION);
            params.put("app", app);
            params.put("commands", new JSONArray());
            params.put("domains", new JSONArray());

            JSONObject message = new JSONObject();
            message.put("jsonrpc", "2.0");
            message.put("method", "server.hello");
            message.put("params", params);

            return message.toString();
        } catch (JSONException e) {
            throw new RuntimeException("Failed to build server.hello", e);
        }
    }

    public String handleMessage(String text) {
        try {
            JSONObject json = new JSONObject(text);
            String method = json.optString("method", "");

            if ("client.hello".equals(method)) {
                return handleClientHello(json);
            }
        } catch (JSONException e) {
            // Malformed JSON — ignore
        }
        return null;
    }

    private String handleClientHello(JSONObject json) throws JSONException {
        int id = json.optInt("id", 0);
        JSONObject params = json.optJSONObject("params");
        if (params == null) {
            return makeError(id, -32600, "Invalid client.hello params", null);
        }

        JSONObject clientProtocol = params.optJSONObject("protocol");
        if (clientProtocol == null) {
            return makeError(id, -32600, "Invalid client.hello params", null);
        }

        int requestedVersion = clientProtocol.optInt("version", 0);
        if (requestedVersion < PROTOCOL_VERSION || requestedVersion > PROTOCOL_VERSION) {
            JSONObject supported = new JSONObject();
            supported.put("min", PROTOCOL_VERSION);
            supported.put("max", PROTOCOL_VERSION);
            JSONObject errorData = new JSONObject();
            errorData.put("supported", supported);

            return makeError(id, -32001,
                "Protocol version " + requestedVersion +
                " not supported. Server supports version " + PROTOCOL_VERSION + ".",
                errorData);
        }

        String sessionId = "sess_" + UUID.randomUUID().toString().substring(0, 8);

        JSONObject negotiated = new JSONObject();
        negotiated.put("protocol", requestedVersion);

        JSONObject result = new JSONObject();
        result.put("sessionId", sessionId);
        result.put("negotiated", negotiated);
        result.put("enabled", new JSONArray());
        result.put("rejected", new JSONArray());

        JSONObject response = new JSONObject();
        response.put("jsonrpc", "2.0");
        response.put("id", id);
        response.put("result", result);

        return response.toString();
    }

    private String makeError(int id, int code, String message, JSONObject data) throws JSONException {
        JSONObject error = new JSONObject();
        error.put("code", code);
        error.put("message", message);
        if (data != null) {
            error.put("data", data);
        }

        JSONObject response = new JSONObject();
        response.put("jsonrpc", "2.0");
        response.put("id", id);
        response.put("error", error);

        return response.toString();
    }
}
