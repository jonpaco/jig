package expo.modules.jig;

import org.json.JSONObject;

public class JigException extends Exception {
    private final int code;
    private final JSONObject data;

    public JigException(int code, String message) {
        this(code, message, null);
    }

    public JigException(int code, String message, JSONObject data) {
        super(message);
        this.code = code;
        this.data = data;
    }

    public int getCode() { return code; }
    public JSONObject getData() { return data; }
}
