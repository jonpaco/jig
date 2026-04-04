package expo.modules.jig;

import org.java_websocket.WebSocket;
import org.java_websocket.handshake.ClientHandshake;
import org.java_websocket.server.WebSocketServer;

import java.net.InetSocketAddress;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

public class JigWebSocketServer extends WebSocketServer {
    private final JigDispatcher dispatcher;
    private final JigAppInfo appInfo;
    private final Map<WebSocket, JigSessionContext> contexts =
        new ConcurrentHashMap<>();
    private final AtomicInteger nextConnectionId = new AtomicInteger(0);

    public JigWebSocketServer(int port, JigDispatcher dispatcher,
                              JigAppInfo appInfo) {
        super(new InetSocketAddress(port));
        this.dispatcher = dispatcher;
        this.appInfo = appInfo;
        setConnectionLostTimeout(0);
        setReuseAddr(true);
    }

    @Override
    public void onStart() {
        System.out.println("[Jig] WebSocket server listening on port "
                           + getPort());
    }

    @Override
    public void onOpen(WebSocket conn, ClientHandshake handshake) {
        System.out.println("[Jig] Client connected from "
                           + conn.getRemoteSocketAddress());
        JigSessionContext context = new JigSessionContext(
            nextConnectionId.getAndIncrement(), appInfo, conn);
        contexts.put(conn, context);
        dispatcher.handleOpen(context);
    }

    @Override
    public void onMessage(WebSocket conn, String message) {
        JigSessionContext context = contexts.get(conn);
        if (context != null) {
            dispatcher.dispatch(message, context);
        }
    }

    @Override
    public void onClose(WebSocket conn, int code, String reason,
                        boolean remote) {
        System.out.println("[Jig] Client disconnected: " + reason);
        contexts.remove(conn);
    }

    @Override
    public void onError(WebSocket conn, Exception ex) {
        System.out.println("[Jig] WebSocket error: " + ex.getMessage());
    }
}
