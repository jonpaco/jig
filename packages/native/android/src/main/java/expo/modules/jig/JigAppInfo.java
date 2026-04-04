package expo.modules.jig;

import org.json.JSONException;
import org.json.JSONObject;

public class JigAppInfo {
    public final String name;
    public final String bundleId;
    public final String platform;
    public final String rnVersion;
    public final String expoVersion; // nullable

    public JigAppInfo(String name, String bundleId, String platform,
                      String rnVersion, String expoVersion) {
        this.name = name;
        this.bundleId = bundleId;
        this.platform = platform;
        this.rnVersion = rnVersion;
        this.expoVersion = expoVersion;
    }

    public JSONObject toJSON() throws JSONException {
        JSONObject obj = new JSONObject();
        obj.put("name", name);
        obj.put("bundleId", bundleId);
        obj.put("platform", platform);
        obj.put("rn", rnVersion);
        if (expoVersion != null) {
            obj.put("expo", expoVersion);
        }
        return obj;
    }
}
