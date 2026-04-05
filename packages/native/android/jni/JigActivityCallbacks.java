package jig;

import android.app.Activity;
import android.app.Application;
import android.os.Bundle;

/**
 * Lifecycle callbacks that relay Activity resume/pause to native code
 * so the screenshot handler can capture the current Activity reference.
 */
public class JigActivityCallbacks implements Application.ActivityLifecycleCallbacks {
    @Override public void onActivityResumed(Activity activity) {
        nativeOnActivityResumed(activity);
    }

    @Override public void onActivityPaused(Activity activity) {
        nativeOnActivityPaused(activity);
    }

    @Override public void onActivityCreated(Activity a, Bundle b) {}
    @Override public void onActivityStarted(Activity a) {}
    @Override public void onActivityStopped(Activity a) {}
    @Override public void onActivitySaveInstanceState(Activity a, Bundle b) {}
    @Override public void onActivityDestroyed(Activity a) {}

    private static native void nativeOnActivityResumed(Activity activity);
    private static native void nativeOnActivityPaused(Activity activity);
}
