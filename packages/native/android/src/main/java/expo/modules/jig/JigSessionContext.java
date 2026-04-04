package expo.modules.jig;

import org.java_websocket.WebSocket;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class JigSessionContext {
    private final int connectionId;
    private final JigAppInfo appInfo;
    private final WebSocket connection;

    private String sessionId;
    private int negotiatedVersion;
    private final Set<String> enabledDomains = new HashSet<>();
    private final Map<String, Object> subscriptions = new HashMap<>();

    public JigSessionContext(int connectionId, JigAppInfo appInfo,
                             WebSocket connection) {
        this.connectionId = connectionId;
        this.appInfo = appInfo;
        this.connection = connection;
    }

    public int getConnectionId() { return connectionId; }
    public JigAppInfo getAppInfo() { return appInfo; }

    public String getSessionId() { return sessionId; }
    public void setSessionId(String sessionId) { this.sessionId = sessionId; }

    public int getNegotiatedVersion() { return negotiatedVersion; }
    public void setNegotiatedVersion(int version) {
        this.negotiatedVersion = version;
    }

    public Set<String> getEnabledDomains() { return enabledDomains; }
    public Map<String, Object> getSubscriptions() { return subscriptions; }

    public void sendText(String text) {
        if (connection.isOpen()) {
            connection.send(text);
        }
    }
}
