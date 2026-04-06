/*
 * JigStandaloneInit.c — JNI entry point for standalone injection
 *
 * When libjig.so is loaded into an Android app via System.loadLibrary("jig")
 * (injected through smali patching), JNI_OnLoad fires automatically.
 * It sets up the C core platform ops and starts the WebSocket server on a
 * background thread.
 */

#include <jni.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <android/log.h>

#include "../../core/jig_server.h"
#include "../../core/jig_dispatcher.h"
#include "../../core/jig_platform.h"
#include "../../core/jig_app_info.h"
#include "../../core/handlers/jig_handshake.h"
#include "../../core/middleware/jig_handshake_gate.h"
#include "../../core/middleware/jig_domain_guard.h"
#include "JigScreenshotShim.h"
#include "../../core/jig_jsbridge.h"
#include "JigElementHandler.h"
#include "JigJSInjector.h"

#define LOG_TAG "Jig"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static jig_server *g_server = NULL;
static JavaVM *g_jvm = NULL;

/* --- Activity lifecycle state --- */
static jobject g_current_activity = NULL;
static pthread_mutex_t g_activity_mutex = PTHREAD_MUTEX_INITIALIZER;

jobject jig_get_current_activity(JNIEnv *env) {
    pthread_mutex_lock(&g_activity_mutex);
    jobject activity = g_current_activity ? (*env)->NewLocalRef(env, g_current_activity) : NULL;
    pthread_mutex_unlock(&g_activity_mutex);
    return activity;
}

JavaVM *jig_get_jvm(void) {
    return g_jvm;
}

/* --- JNI cached classes/methods --- */
static jclass g_runner_class = NULL;
static jmethodID g_post_method = NULL;

/* --- Main thread dispatch --- */

JNIEXPORT void JNICALL
Java_jig_JigMainThreadRunner_executePendingCallback(JNIEnv *env, jclass cls,
                                                     jlong fnPtr, jlong ctxPtr) {
    (void)env; (void)cls;
    void (*callback)(void *) = (void (*)(void *))(uintptr_t)fnPtr;
    void *ctx = (void *)(uintptr_t)ctxPtr;
    if (callback) {
        callback(ctx);
    }
}

static void android_run_on_main_thread(void (*callback)(void *ctx), void *ctx) {
    JNIEnv *env;
    int attached = 0;

    jint rc = (*g_jvm)->GetEnv(g_jvm, (void **)&env, JNI_VERSION_1_6);
    if (rc == JNI_EDETACHED) {
        (*g_jvm)->AttachCurrentThread(g_jvm, &env, NULL);
        attached = 1;
    }

    if (g_runner_class && g_post_method) {
        (*env)->CallStaticVoidMethod(env, g_runner_class, g_post_method,
                                      (jlong)(uintptr_t)callback,
                                      (jlong)(uintptr_t)ctx);
    } else {
        LOGE("JigMainThreadRunner not available, executing synchronously");
        callback(ctx);
    }

    if (attached) {
        (*g_jvm)->DetachCurrentThread(g_jvm);
    }
}

/* --- Activity lifecycle callbacks (called from Java proxy) --- */

JNIEXPORT void JNICALL
Java_jig_JigActivityCallbacks_nativeOnActivityResumed(JNIEnv *env, jclass cls, jobject activity) {
    (void)cls;
    pthread_mutex_lock(&g_activity_mutex);
    if (g_current_activity) {
        (*env)->DeleteGlobalRef(env, g_current_activity);
    }
    g_current_activity = (*env)->NewGlobalRef(env, activity);
    pthread_mutex_unlock(&g_activity_mutex);
    LOGI("Activity resumed — captured reference");
}

JNIEXPORT void JNICALL
Java_jig_JigActivityCallbacks_nativeOnActivityPaused(JNIEnv *env, jclass cls, jobject activity) {
    (void)cls;
    pthread_mutex_lock(&g_activity_mutex);
    if (g_current_activity && (*env)->IsSameObject(env, g_current_activity, activity)) {
        (*env)->DeleteGlobalRef(env, g_current_activity);
        g_current_activity = NULL;
        LOGI("Activity paused — cleared reference");
    }
    pthread_mutex_unlock(&g_activity_mutex);
}

/* --- Platform ops -------------------------------------------------------- */

static void android_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_INFO, LOG_TAG, fmt, args);
    va_end(args);
}

static int android_get_app_info(jig_app_info *info) {
    info->name = strdup("Unknown");
    info->bundle_id = strdup("unknown");
    info->platform = strdup("android");
    info->rn_version = strdup("unknown");
    info->expo_version = NULL;

    FILE *f = fopen("/proc/self/cmdline", "r");
    if (f) {
        char buf[256] = {0};
        size_t n = fread(buf, 1, sizeof(buf) - 1, f);
        fclose(f);
        if (n > 0) {
            buf[n] = '\0';
            free(info->bundle_id);
            info->bundle_id = strdup(buf);
            free(info->name);
            info->name = strdup(buf);
        }
    }
    return 0;
}

/* --- Server thread ------------------------------------------------------- */

static void *server_thread(void *arg) {
    jig_server *server = (jig_server *)arg;
    LOGI("Server thread starting");
    int rc = jig_server_start(server);
    if (rc != 0) {
        LOGE("Server exited with error code %d", rc);
    }
    return NULL;
}

/* --- JNI registration ---------------------------------------------------- */

static JNINativeMethod sRunnerMethods[] = {
    {"executePendingCallback", "(JJ)V",
     (void *)Java_jig_JigMainThreadRunner_executePendingCallback},
};

static JNINativeMethod sActivityMethods[] = {
    {"nativeOnActivityResumed", "(Landroid/app/Activity;)V",
     (void *)Java_jig_JigActivityCallbacks_nativeOnActivityResumed},
    {"nativeOnActivityPaused", "(Landroid/app/Activity;)V",
     (void *)Java_jig_JigActivityCallbacks_nativeOnActivityPaused},
};

static int register_natives(JNIEnv *env) {
    /* JigMainThreadRunner */
    jclass runner = (*env)->FindClass(env, "jig/JigMainThreadRunner");
    if (!runner) {
        LOGE("Cannot find jig/JigMainThreadRunner class");
        return -1;
    }
    g_runner_class = (*env)->NewGlobalRef(env, runner);
    g_post_method = (*env)->GetStaticMethodID(env, g_runner_class,
                                               "postToMainThread", "(JJ)V");
    if (!g_post_method) {
        LOGE("Cannot find postToMainThread method");
        return -1;
    }
    if ((*env)->RegisterNatives(env, g_runner_class, sRunnerMethods, 1) < 0) {
        LOGE("Failed to register JigMainThreadRunner natives");
        return -1;
    }

    /* JigActivityCallbacks */
    jclass callbacks = (*env)->FindClass(env, "jig/JigActivityCallbacks");
    if (!callbacks) {
        LOGE("Cannot find jig/JigActivityCallbacks class");
        return -1;
    }
    if ((*env)->RegisterNatives(env, callbacks, sActivityMethods, 2) < 0) {
        LOGE("Failed to register JigActivityCallbacks natives");
        return -1;
    }

    return 0;
}

static void register_activity_callbacks(JNIEnv *env) {
    /* Get Application via ActivityThread.currentApplication() */
    jclass at_class = (*env)->FindClass(env, "android/app/ActivityThread");
    if (!at_class) {
        LOGE("Cannot find ActivityThread class");
        return;
    }
    jmethodID current_app = (*env)->GetStaticMethodID(
        env, at_class, "currentApplication", "()Landroid/app/Application;");
    if (!current_app) {
        LOGE("Cannot find currentApplication method");
        return;
    }
    jobject application = (*env)->CallStaticObjectMethod(env, at_class, current_app);
    if (!application) {
        LOGE("currentApplication() returned null");
        return;
    }

    /* Create JigActivityCallbacks instance and register */
    jclass cb_class = (*env)->FindClass(env, "jig/JigActivityCallbacks");
    if (!cb_class) {
        LOGE("Cannot find JigActivityCallbacks class");
        return;
    }
    jmethodID cb_init = (*env)->GetMethodID(env, cb_class, "<init>", "()V");
    jobject cb_instance = (*env)->NewObject(env, cb_class, cb_init);

    jclass app_class = (*env)->FindClass(env, "android/app/Application");
    jmethodID register_method = (*env)->GetMethodID(
        env, app_class, "registerActivityLifecycleCallbacks",
        "(Landroid/app/Application$ActivityLifecycleCallbacks;)V");
    (*env)->CallVoidMethod(env, application, register_method, cb_instance);

    LOGI("Activity lifecycle callbacks registered");
}

/* --- JNI entry point ----------------------------------------------------- */

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    (void)reserved;
    g_jvm = vm;

    JNIEnv *env;
    (*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_6);

    /* Register native methods for our Java helpers */
    if (register_natives(env) != 0) {
        LOGE("Failed to register native methods");
    }

    /* Register activity lifecycle callbacks */
    register_activity_callbacks(env);

    /* Platform ops */
    static jig_platform_ops ops = {
        .screenshot = NULL,
        .get_app_info = android_get_app_info,
        .log = android_log,
        .run_on_main_thread = android_run_on_main_thread,
    };
    jig_platform_set_ops(&ops);

    /* Build app info */
    jig_app_info proc_info = {0};
    android_get_app_info(&proc_info);

    jig_app_info *app_info = jig_app_info_create(
        proc_info.name,
        proc_info.bundle_id,
        proc_info.platform,
        proc_info.rn_version,
        proc_info.expo_version
    );

    free(proc_info.name);
    free(proc_info.bundle_id);
    free(proc_info.platform);
    free(proc_info.rn_version);

    /* JS bridge for fiber data */
    static jig_jsbridge *jsbridge = NULL;
    jsbridge = jig_jsbridge_create();
    jig_handler **bridge_handlers = jig_jsbridge_get_handlers(jsbridge);

    /* Handlers — must be static since the dispatcher stores pointers, not copies */
    static jig_handler *handlers[6];
    handlers[0] = jig_handshake_create();
    handlers[1] = jig_screenshot_handler_create();
    handlers[2] = jig_android_find_element_handler_create(jsbridge);
    handlers[3] = jig_android_find_elements_handler_create(jsbridge);
    handlers[4] = bridge_handlers[0]; /* jig.internal.ready */
    handlers[5] = bridge_handlers[1]; /* jig.internal.fiberData */

    /* Middleware — must be static for the same reason */
    static jig_middleware middlewares[2];
    middlewares[0] = jig_handshake_gate_create();
    middlewares[1] = jig_domain_guard_create(NULL, 0);

    /* Dispatcher */
    jig_dispatcher_config *dispatcher = jig_dispatcher_create(
        middlewares, 2,
        handlers, 6,
        NULL, 0
    );

    /* Create server */
    g_server = jig_server_create(4042, dispatcher, app_info);
    if (!g_server) {
        LOGE("Failed to create Jig server");
        return JNI_VERSION_1_6;
    }

    /* Start on background thread */
    pthread_t tid;
    int err = pthread_create(&tid, NULL, server_thread, g_server);
    if (err != 0) {
        LOGE("Failed to create server thread: %d", err);
        jig_server_destroy(g_server);
        g_server = NULL;
        return JNI_VERSION_1_6;
    }
    pthread_detach(tid);

    /* Start JS injector (polls for ReactContext, injects fiber walker) */
    jig_android_start_js_injector(env);

    LOGI("Jig standalone server started on port 4042");

    return JNI_VERSION_1_6;
}
