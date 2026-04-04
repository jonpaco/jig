package expo.modules.jig.middleware;

import expo.modules.jig.JigErrors;
import expo.modules.jig.JigException;
import expo.modules.jig.JigMiddleware;
import expo.modules.jig.JigSessionContext;

import org.json.JSONObject;

/** Rejects any command received before handshake completes (except client.hello). */
public class HandshakeGate implements JigMiddleware {

    @Override
    public void validate(String method, JSONObject params,
                         JigSessionContext context) throws JigException {
        if ("client.hello".equals(method)) {
            return;
        }
        if (context.getSessionId() == null) {
            throw JigErrors.handshakeRequired();
        }
    }
}
