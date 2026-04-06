#include "JigJSInjector.h"
#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define LOG_TAG "Jig"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern JavaVM *jig_get_jvm(void);

static const char *FIBER_WALKER_PATH = "/data/local/tmp/jig-fiber-walker.js";

static void *js_injector_thread(void *arg) {
    (void)arg;
    JavaVM *jvm = jig_get_jvm();
    if (!jvm) {
        LOGE("JS injector: no JVM reference");
        return NULL;
    }

    JNIEnv *env;
    (*jvm)->AttachCurrentThread(jvm, &env, NULL);

    int attempts = 60;
    jobject reactContext = NULL;

    while (attempts-- > 0 && !reactContext) {
        usleep(500000);

        jclass atClass = (*env)->FindClass(env, "android/app/ActivityThread");
        if (!atClass || (*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
            continue;
        }

        jmethodID currentApp = (*env)->GetStaticMethodID(
            env, atClass, "currentApplication", "()Landroid/app/Application;");
        jobject app = (*env)->CallStaticObjectMethod(env, atClass, currentApp);
        (*env)->DeleteLocalRef(env, atClass);
        if (!app) continue;

        jclass reactAppClass = (*env)->FindClass(
            env, "com/facebook/react/ReactApplication");
        if (!reactAppClass || (*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
            (*env)->DeleteLocalRef(env, app);
            continue;
        }

        if (!(*env)->IsInstanceOf(env, app, reactAppClass)) {
            (*env)->DeleteLocalRef(env, reactAppClass);
            (*env)->DeleteLocalRef(env, app);
            continue;
        }

        jmethodID getHost = (*env)->GetMethodID(
            env, reactAppClass, "getReactNativeHost",
            "()Lcom/facebook/react/ReactNativeHost;");
        jobject host = (*env)->CallObjectMethod(env, app, getHost);
        (*env)->DeleteLocalRef(env, reactAppClass);
        (*env)->DeleteLocalRef(env, app);
        if (!host || (*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
            continue;
        }

        jclass hostClass = (*env)->GetObjectClass(env, host);
        jmethodID getManager = (*env)->GetMethodID(
            env, hostClass, "getReactInstanceManager",
            "()Lcom/facebook/react/ReactInstanceManager;");
        jobject manager = (*env)->CallObjectMethod(env, host, getManager);
        (*env)->DeleteLocalRef(env, hostClass);
        (*env)->DeleteLocalRef(env, host);
        if (!manager || (*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
            continue;
        }

        jclass managerClass = (*env)->GetObjectClass(env, manager);
        jmethodID getContext = (*env)->GetMethodID(
            env, managerClass, "getCurrentReactContext",
            "()Lcom/facebook/react/bridge/ReactContext;");
        jobject ctx = (*env)->CallObjectMethod(env, manager, getContext);
        (*env)->DeleteLocalRef(env, managerClass);
        (*env)->DeleteLocalRef(env, manager);
        if (ctx && !(*env)->ExceptionCheck(env)) {
            reactContext = ctx;
        } else {
            (*env)->ExceptionClear(env);
        }
    }

    if (!reactContext) {
        LOGI("JS injector: ReactContext not available after polling — "
             "component selectors will be unavailable");
        (*jvm)->DetachCurrentThread(jvm);
        return NULL;
    }

    LOGI("JS injector: ReactContext found, injecting fiber walker");

    jclass ctxClass = (*env)->GetObjectClass(env, reactContext);
    jmethodID getCatalyst = (*env)->GetMethodID(
        env, ctxClass, "getCatalystInstance",
        "()Lcom/facebook/react/bridge/CatalystInstance;");
    jobject catalyst = (*env)->CallObjectMethod(env, reactContext, getCatalyst);
    (*env)->DeleteLocalRef(env, ctxClass);
    (*env)->DeleteLocalRef(env, reactContext);

    if (!catalyst || (*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        LOGE("JS injector: failed to get CatalystInstance");
        (*jvm)->DetachCurrentThread(jvm);
        return NULL;
    }

    FILE *f = fopen(FIBER_WALKER_PATH, "r");
    if (!f) {
        LOGE("JS injector: fiber-walker.js not found at %s", FIBER_WALKER_PATH);
        (*env)->DeleteLocalRef(env, catalyst);
        (*jvm)->DetachCurrentThread(jvm);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *jsCode = malloc((size_t)size + 1);
    fread(jsCode, 1, (size_t)size, f);
    jsCode[size] = '\0';
    fclose(f);

    jclass catalystClass = (*env)->GetObjectClass(env, catalyst);
    jmethodID loadScript = (*env)->GetMethodID(
        env, catalystClass, "loadScriptFromFile",
        "(Ljava/lang/String;Ljava/lang/String;Z)V");
    if (loadScript && !(*env)->ExceptionCheck(env)) {
        jstring filePath = (*env)->NewStringUTF(env, FIBER_WALKER_PATH);
        jstring sourceUrl = (*env)->NewStringUTF(env, "jig://fiber-walker.js");
        (*env)->CallVoidMethod(env, catalyst, loadScript,
                               filePath, sourceUrl, JNI_FALSE);
        (*env)->DeleteLocalRef(env, filePath);
        (*env)->DeleteLocalRef(env, sourceUrl);

        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
            LOGE("JS injector: loadScriptFromFile failed");
        } else {
            LOGI("JS injector: fiber walker injected via loadScriptFromFile");
        }
    } else {
        (*env)->ExceptionClear(env);
        LOGI("JS injector: loadScriptFromFile not available — "
             "component selectors may be unavailable");
    }

    free(jsCode);
    (*env)->DeleteLocalRef(env, catalystClass);
    (*env)->DeleteLocalRef(env, catalyst);
    (*jvm)->DetachCurrentThread(jvm);

    return NULL;
}

void jig_android_start_js_injector(JNIEnv *env) {
    (void)env;
    pthread_t tid;
    int err = pthread_create(&tid, NULL, js_injector_thread, NULL);
    if (err != 0) {
        LOGE("JS injector: failed to create thread: %d", err);
        return;
    }
    pthread_detach(tid);
    LOGI("JS injector: polling thread started");
}
