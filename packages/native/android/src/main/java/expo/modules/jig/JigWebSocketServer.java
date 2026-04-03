package expo.modules.jig;

import org.java_websocket.WebSocket;
import org.java_websocket.handshake.ClientHandshake;
import org.java_websocket.server.WebSocketServer;

import java.net.InetSocketAddress;

public class JigWebSocketServer extends WebSocketServer {
    private final JigHandshakeHandler handler;

    public JigWebSocketServer(int port, JigHandshakeHandler handler) {
        super(new InetSocketAddress(port));
        this.handler = handler;
        setConnectionLostTimeout(0);
        setReuseAddr(true);
    }

    @Override
    public void onStart() {
        System.out.println("[Jig] WebSocket server listening on port " + getPort());
    }

    @Override
    public void onOpen(WebSocket conn, ClientHandshake handshake) {
        System.out.println("[Jig] Client connected from " + conn.getRemoteSocketAddress());
        conn.send(handler.makeServerHello());
    }

    @Override
    public void onMessage(WebSocket conn, String message) {
        String response = handler.handleMessage(message);
        if (response != null && conn.isOpen()) {
            conn.send(response);
        }
    }

    @Override
    public void onClose(WebSocket conn, int code, String reason, boolean remote) {
        System.out.println("[Jig] Client disconnected: " + reason);
    }

    @Override
    public void onError(WebSocket conn, Exception ex) {
        System.out.println("[Jig] WebSocket error: " + ex.getMessage());
    }
}
