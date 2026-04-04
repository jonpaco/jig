package expo.modules.jig;

import org.json.JSONException;
import org.json.JSONObject;

public class JigErrors {
    public static final int PARSE_ERROR         = -32700;
    public static final int INVALID_REQUEST     = -32600;
    public static final int METHOD_NOT_FOUND    = -32601;
    public static final int INVALID_PARAMS      = -32602;
    public static final int INTERNAL_ERROR      = -32603;
    public static final int HANDSHAKE_REQUIRED  = -32000;
    public static final int PROTOCOL_VERSION    = -32001;
    public static final int ELEMENT_NOT_FOUND   = -32002;
    public static final int TIMEOUT             = -32003;
    public static final int INVALID_SELECTOR    = -32004;
    public static final int UNSUPPORTED_DOMAIN  = -32005;
    public static final int DOMAIN_NOT_ENABLED  = -32006;

    public static JigException handshakeRequired() {
        return new JigException(HANDSHAKE_REQUIRED,
            "Handshake required before sending commands");
    }

    public static JigException protocolVersionError(int requested,
                                                     int min, int max) {
        try {
            JSONObject supported = new JSONObject();
            supported.put("min", min);
            supported.put("max", max);
            JSONObject data = new JSONObject();
            data.put("supported", supported);
            return new JigException(PROTOCOL_VERSION,
                "Protocol version " + requested +
                " not supported. Server supports version " + min + ".",
                data);
        } catch (JSONException e) {
            return new JigException(PROTOCOL_VERSION,
                "Protocol version not supported");
        }
    }

    public static JigException unsupportedDomain(String domain) {
        return new JigException(UNSUPPORTED_DOMAIN,
            "Domain '" + domain + "' is not supported");
    }

    public static JigException methodNotFound(String method) {
        return new JigException(METHOD_NOT_FOUND,
            "Method '" + method + "' not found");
    }

    public static JigException invalidParams(String message) {
        return new JigException(INVALID_PARAMS, message);
    }

    public static JigException internalError(String message) {
        return new JigException(INTERNAL_ERROR, message);
    }

    public static JigException parseError(String message) {
        return new JigException(PARSE_ERROR, message);
    }

    public static JigException invalidRequest(String message) {
        return new JigException(INVALID_REQUEST, message);
    }

    public static JigException elementNotFound(String message,
                                                JSONObject data) {
        return new JigException(ELEMENT_NOT_FOUND, message, data);
    }

    public static JigException timeout(String message) {
        return new JigException(TIMEOUT, message);
    }

    public static JigException invalidSelector(String message) {
        return new JigException(INVALID_SELECTOR, message);
    }

    public static JigException domainNotEnabled(String domain) {
        return new JigException(DOMAIN_NOT_ENABLED,
            "Domain '" + domain + "' must be enabled before use");
    }
}
