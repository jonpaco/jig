package expo.modules.jig.handlers;

import expo.modules.jig.JigErrors;
import expo.modules.jig.JigException;
import expo.modules.jig.JigHandler;
import expo.modules.jig.JigSessionContext;
import expo.modules.jig.JigThreadTarget;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.UUID;

public class HandshakeHandler implements JigHandler {
    private static final int PROTOCOL_VERSION = 1;

    @Override
    public String getMethod() {
        return "client.hello";
    }

    @Override
    public JigThreadTarget getThreadTarget() {
        return JigThreadTarget.WEBSOCKET;
    }

    @Override
    public Object handle(JSONObject params, JigSessionContext context)
            throws JigException {
        if (params == null) {
            throw JigErrors.invalidParams("Invalid client.hello params");
        }

        JSONObject clientProtocol = params.optJSONObject("protocol");
        if (clientProtocol == null) {
            throw JigErrors.invalidParams("Invalid client.hello params");
        }

        int requestedVersion = clientProtocol.optInt("version", 0);
        if (requestedVersion == 0) {
            throw JigErrors.invalidParams("Invalid client.hello params");
        }

        if (requestedVersion < PROTOCOL_VERSION ||
            requestedVersion > PROTOCOL_VERSION) {
            throw JigErrors.protocolVersionError(
                requestedVersion, PROTOCOL_VERSION, PROTOCOL_VERSION);
        }

        String sessionId = "sess_" +
            UUID.randomUUID().toString().substring(0, 8);

        try {
            JSONObject negotiated = new JSONObject();
            negotiated.put("protocol", requestedVersion);

            JSONObject result = new JSONObject();
            result.put("sessionId", sessionId);
            result.put("negotiated", negotiated);
            result.put("enabled", new JSONArray());
            result.put("rejected", new JSONArray());

            return result;
        } catch (JSONException e) {
            throw JigErrors.internalError("Failed to build response");
        }
    }
}
