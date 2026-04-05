package jig;

import android.os.Handler;
import android.os.Looper;

/**
 * Posts native C callbacks to the Android main (UI) thread.
 * Called from JNI — the native side stores function pointer + context,
 * then calls postToMainThread() which posts a Runnable that calls back
 * into C via the native executePendingCallback() method.
 */
public class JigMainThreadRunner {
    private static final Handler sMainHandler = new Handler(Looper.getMainLooper());

    /**
     * Called from JNI. Posts a Runnable to the main thread that will
     * call executePendingCallback() with the stored pointers.
     */
    public static void postToMainThread(long fnPtr, long ctxPtr) {
        final long fn = fnPtr;
        final long ctx = ctxPtr;
        sMainHandler.post(new Runnable() {
            @Override
            public void run() {
                executePendingCallback(fn, ctx);
            }
        });
    }

    /**
     * Native method — implemented in JigStandaloneInit.c.
     * Casts fnPtr back to a function pointer and calls it with ctxPtr.
     */
    private static native void executePendingCallback(long fnPtr, long ctxPtr);
}
