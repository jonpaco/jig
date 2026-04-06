package jig;

import android.graphics.Bitmap;
import android.os.Handler;
import android.os.HandlerThread;
import android.view.PixelCopy;
import android.view.Window;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Synchronous PixelCopy wrapper for the C screenshot shim.
 * Called from JNI — captures the window's hardware-rendered surface
 * into the provided bitmap. Returns the PixelCopy result code.
 *
 * Uses a background HandlerThread for the PixelCopy callback and
 * blocks the calling thread (main thread) with a CountDownLatch.
 * This is safe because PixelCopy reads from SurfaceFlinger and does
 * not need the main thread to complete.
 */
public class JigScreenshotHelper {

    /**
     * Capture the window contents into the provided bitmap.
     * Must be called with a pre-allocated ARGB_8888 bitmap matching
     * the window dimensions.
     *
     * @return PixelCopy result code (0 = SUCCESS)
     */
    public static int capture(Window window, Bitmap bitmap) {
        HandlerThread thread = new HandlerThread("jig-screenshot");
        thread.start();

        CountDownLatch latch = new CountDownLatch(1);
        final int[] result = new int[]{-1};

        PixelCopy.request(window, bitmap, new PixelCopy.OnPixelCopyFinishedListener() {
            @Override
            public void onPixelCopyFinished(int copyResult) {
                result[0] = copyResult;
                latch.countDown();
            }
        }, new Handler(thread.getLooper()));

        try {
            latch.await(5, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            // result stays -1
        } finally {
            thread.quit();
        }

        return result[0];
    }
}
