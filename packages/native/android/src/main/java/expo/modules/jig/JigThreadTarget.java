package expo.modules.jig;

public enum JigThreadTarget {
    /** Run on the WebSocket thread (default, cheapest) */
    WEBSOCKET,
    /** Dispatch to main/UI thread */
    MAIN,
}
