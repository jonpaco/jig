package expo.modules.jig.handlers;

import android.app.Activity;
import android.graphics.Bitmap;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Base64;
import android.view.PixelCopy;
import android.view.Window;

import expo.modules.jig.JigErrors;
import expo.modules.jig.JigException;
import expo.modules.jig.JigHandler;
import expo.modules.jig.JigSessionContext;
import expo.modules.jig.JigThreadTarget;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class ScreenshotHandler implements JigHandler {

    public interface ActivityProvider {
        Activity getActivity();
    }

    private final ActivityProvider activityProvider;

    public ScreenshotHandler(ActivityProvider provider) {
        this.activityProvider = provider;
    }

    @Override
    public String getMethod() {
        return "jig.screenshot";
    }

    @Override
    public JigThreadTarget getThreadTarget() {
        return JigThreadTarget.MAIN;
    }

    @Override
    public Object handle(JSONObject params, JigSessionContext context)
            throws JigException {

        Activity activity = activityProvider.getActivity();
        if (activity == null) {
            throw JigErrors.internalError(
                "No active activity available for screenshot");
        }

        Window window = activity.getWindow();
        if (window == null || window.getDecorView() == null) {
            throw JigErrors.internalError(
                "No active window available for screenshot");
        }

        int width = window.getDecorView().getWidth();
        int height = window.getDecorView().getHeight();

        if (width == 0 || height == 0) {
            throw JigErrors.internalError(
                "Window has zero dimensions");
        }

        String format = params != null ? params.optString("format", "png") : "png";
        int quality = params != null ? params.optInt("quality", 80) : 80;

        // Capture via PixelCopy (API 26+)
        Bitmap bitmap = Bitmap.createBitmap(width, height,
            Bitmap.Config.ARGB_8888);

        HandlerThread callbackThread = new HandlerThread("jig-screenshot");
        callbackThread.start();

        CountDownLatch latch = new CountDownLatch(1);
        int[] copyResult = new int[1];

        PixelCopy.request(window, bitmap, result -> {
            copyResult[0] = result;
            latch.countDown();
        }, new Handler(callbackThread.getLooper()));

        try {
            boolean completed = latch.await(5, TimeUnit.SECONDS);
            if (!completed) {
                bitmap.recycle();
                throw JigErrors.timeout("Screenshot capture timed out");
            }
        } catch (InterruptedException e) {
            bitmap.recycle();
            throw JigErrors.internalError("Screenshot capture interrupted");
        } finally {
            callbackThread.quit();
        }

        if (copyResult[0] != PixelCopy.SUCCESS) {
            bitmap.recycle();
            throw JigErrors.internalError(
                "PixelCopy failed with code " + copyResult[0]);
        }

        // Encode
        ByteArrayOutputStream stream = new ByteArrayOutputStream();
        String actualFormat;
        if ("jpeg".equals(format)) {
            bitmap.compress(Bitmap.CompressFormat.JPEG, quality, stream);
            actualFormat = "jpeg";
        } else {
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, stream);
            actualFormat = "png";
        }

        String base64 = Base64.encodeToString(
            stream.toByteArray(), Base64.NO_WRAP);
        long timestamp = System.currentTimeMillis();

        bitmap.recycle();

        try {
            JSONObject result = new JSONObject();
            result.put("data", base64);
            result.put("format", actualFormat);
            result.put("width", width);
            result.put("height", height);
            result.put("timestamp", timestamp);
            return result;
        } catch (JSONException e) {
            throw JigErrors.internalError("Failed to build screenshot response");
        }
    }
}
