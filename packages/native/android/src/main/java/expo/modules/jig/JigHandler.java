package expo.modules.jig;

import org.json.JSONObject;

public interface JigHandler {

    /** The JSON-RPC method this handler responds to (e.g., "client.hello") */
    String getMethod();

    /** Where this handler must execute */
    JigThreadTarget getThreadTarget();

    /**
     * Handle the command.
     * @param params  The parsed "params" object (may be null)
     * @param context The session context for this connection
     * @return The result object for the JSON-RPC response
     * @throws JigException on failure — the dispatcher formats the error response
     */
    Object handle(JSONObject params, JigSessionContext context) throws JigException;
}
