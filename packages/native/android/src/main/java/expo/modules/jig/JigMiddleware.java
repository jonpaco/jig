package expo.modules.jig;

import org.json.JSONObject;

public interface JigMiddleware {

    /**
     * Validate the incoming message.
     * @param method  The JSON-RPC method name
     * @param params  The parsed params (may be null)
     * @param context The session context
     * @throws JigException to reject the message
     */
    void validate(String method, JSONObject params, JigSessionContext context)
        throws JigException;
}
