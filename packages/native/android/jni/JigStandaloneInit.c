/*
 * JigStandaloneInit.c — JNI entry point for standalone injection
 *
 * When libjig.so is loaded into an Android app via System.loadLibrary("jig")
 * (injected through smali patching), JNI_OnLoad fires automatically.
 * It sets up the C core platform ops and starts the WebSocket server on a
 * background thread.
 *
 * Screenshot handler intentionally deferred from Plan 3 scope.
 * JNI screenshot requires an Activity context that is not available
 * at JNI_OnLoad time. Will be implemented when Activity lifecycle
 * hooks are added (see deferred work in migration spec).
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

#define LOG_TAG "Jig"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static jig_server *g_server = NULL;
static JavaVM *g_jvm = NULL;

/* --- Platform ops -------------------------------------------------------- */

static void android_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_INFO, LOG_TAG, fmt, args);
    va_end(args);
}

static int android_get_app_info(jig_app_info *info) {
    /*
     * For standalone injection we read the package name from
     * /proc/self/cmdline — the kernel writes the process name there,
     * which for Android apps is the package name.
     */
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

/* --- JNI entry point ----------------------------------------------------- */

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    (void)reserved;
    g_jvm = vm;

    /* Platform ops — no screenshot for standalone injection */
    static jig_platform_ops ops = {
        .screenshot = NULL,
        .get_app_info = android_get_app_info,
        .log = android_log,
    };
    jig_platform_set_ops(&ops);

    /* Build app info from /proc/self/cmdline */
    jig_app_info proc_info = {0};
    android_get_app_info(&proc_info);

    jig_app_info *app_info = jig_app_info_create(
        proc_info.name,
        proc_info.bundle_id,
        proc_info.platform,
        proc_info.rn_version,
        proc_info.expo_version
    );

    /* Free the temporary strings from android_get_app_info */
    free(proc_info.name);
    free(proc_info.bundle_id);
    free(proc_info.platform);
    free(proc_info.rn_version);

    /* Handlers */
    jig_handler *handshake = jig_handshake_create();
    jig_handler *handlers[] = { handshake };

    /* Middleware */
    jig_middleware gate = jig_handshake_gate_create();
    jig_middleware guard = jig_domain_guard_create(NULL, 0);
    jig_middleware middlewares[] = { gate, guard };

    /* Dispatcher */
    jig_dispatcher_config *dispatcher = jig_dispatcher_create(
        middlewares, 2,
        handlers, 1,
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

    LOGI("Jig standalone server started on port 4042");

    return JNI_VERSION_1_6;
}
