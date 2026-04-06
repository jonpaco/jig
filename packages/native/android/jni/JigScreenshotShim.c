/*
 * JigScreenshotShim.c — Android screenshot via View.draw + JNI
 *
 * Must run on the main thread (JIG_THREAD_MAIN). Requires an Activity
 * reference captured by the lifecycle callbacks in JigStandaloneInit.c.
 */

#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <android/log.h>

#include "JigScreenshotShim.h"
#include "../../core/jig_handler.h"
#include "../../core/jig_errors.h"
#include "../../core/vendor/cJSON/cJSON.h"

#define LOG_TAG "Jig"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* Defined in JigStandaloneInit.c */
extern jobject jig_get_current_activity(JNIEnv *env);
extern JavaVM *jig_get_jvm(void);

/* --- Cached JNI references (initialized on first use) --- */
static int s_jni_cached = 0;
static jclass s_bitmap_class = NULL;
static jmethodID s_bitmap_create = NULL;
static jmethodID s_bitmap_compress = NULL;
static jmethodID s_bitmap_getWidth = NULL;
static jmethodID s_bitmap_getHeight = NULL;
static jmethodID s_bitmap_recycle = NULL;

static jclass s_bitmap_config_class = NULL;
static jfieldID s_argb8888_field = NULL;

static jclass s_baos_class = NULL;
static jmethodID s_baos_init = NULL;
static jmethodID s_baos_toByteArray = NULL;

static jclass s_compress_format_class = NULL;
static jfieldID s_jpeg_field = NULL;
static jfieldID s_png_field = NULL;

static jclass s_base64_class = NULL;
static jmethodID s_base64_encodeToString = NULL;

static int cache_jni_refs(JNIEnv *env) {
    if (s_jni_cached) return 0;

    #define CACHE_CLASS(var, name) do { \
        jclass cls = (*env)->FindClass(env, name); \
        if (!cls) { LOGE("Cannot find class: %s", name); return -1; } \
        var = (*env)->NewGlobalRef(env, cls); \
        (*env)->DeleteLocalRef(env, cls); \
    } while(0)

    CACHE_CLASS(s_bitmap_class, "android/graphics/Bitmap");
    s_bitmap_create = (*env)->GetStaticMethodID(env, s_bitmap_class,
        "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    s_bitmap_compress = (*env)->GetMethodID(env, s_bitmap_class,
        "compress", "(Landroid/graphics/Bitmap$CompressFormat;ILjava/io/OutputStream;)Z");
    s_bitmap_getWidth = (*env)->GetMethodID(env, s_bitmap_class, "getWidth", "()I");
    s_bitmap_getHeight = (*env)->GetMethodID(env, s_bitmap_class, "getHeight", "()I");
    s_bitmap_recycle = (*env)->GetMethodID(env, s_bitmap_class, "recycle", "()V");

    CACHE_CLASS(s_bitmap_config_class, "android/graphics/Bitmap$Config");
    s_argb8888_field = (*env)->GetStaticFieldID(env, s_bitmap_config_class,
        "ARGB_8888", "Landroid/graphics/Bitmap$Config;");

    CACHE_CLASS(s_baos_class, "java/io/ByteArrayOutputStream");
    s_baos_init = (*env)->GetMethodID(env, s_baos_class, "<init>", "()V");
    s_baos_toByteArray = (*env)->GetMethodID(env, s_baos_class, "toByteArray", "()[B");

    CACHE_CLASS(s_compress_format_class, "android/graphics/Bitmap$CompressFormat");
    s_jpeg_field = (*env)->GetStaticFieldID(env, s_compress_format_class,
        "JPEG", "Landroid/graphics/Bitmap$CompressFormat;");
    s_png_field = (*env)->GetStaticFieldID(env, s_compress_format_class,
        "PNG", "Landroid/graphics/Bitmap$CompressFormat;");

    CACHE_CLASS(s_base64_class, "android/util/Base64");
    s_base64_encodeToString = (*env)->GetStaticMethodID(env, s_base64_class,
        "encodeToString", "([BI)Ljava/lang/String;");

    #undef CACHE_CLASS
    s_jni_cached = 1;
    return 0;
}

static cJSON *screenshot_handle(jig_handler *self, cJSON *params,
                                 jig_session *session, jig_error **err) {
    (void)self; (void)session;

    /* Parse params */
    const char *format_str = "png";
    int quality = 80;
    if (params) {
        cJSON *fmt = cJSON_GetObjectItemCaseSensitive(params, "format");
        if (cJSON_IsString(fmt) && fmt->valuestring) {
            format_str = fmt->valuestring;
        }
        cJSON *q = cJSON_GetObjectItemCaseSensitive(params, "quality");
        if (cJSON_IsNumber(q)) {
            quality = q->valueint;
        }
    }

    JavaVM *jvm = jig_get_jvm();
    if (!jvm) {
        *err = jig_error_internal("JVM not available");
        return NULL;
    }

    JNIEnv *env;
    int attached = 0;
    jint rc = (*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_6);
    if (rc == JNI_EDETACHED) {
        (*jvm)->AttachCurrentThread(jvm, &env, NULL);
        attached = 1;
    }

    if (cache_jni_refs(env) != 0) {
        if (attached) (*jvm)->DetachCurrentThread(jvm);
        *err = jig_error_internal("Failed to cache JNI references");
        return NULL;
    }

    /* Get current Activity */
    jobject activity = jig_get_current_activity(env);
    if (!activity) {
        if (attached) (*jvm)->DetachCurrentThread(jvm);
        *err = jig_error_internal("No active Activity for screenshot");
        return NULL;
    }

    /* Get Window from Activity */
    jclass activity_class = (*env)->GetObjectClass(env, activity);
    jmethodID getWindow = (*env)->GetMethodID(env, activity_class,
        "getWindow", "()Landroid/view/Window;");
    jobject window = (*env)->CallObjectMethod(env, activity, getWindow);
    if (!window) {
        (*env)->DeleteLocalRef(env, activity);
        if (attached) (*jvm)->DetachCurrentThread(jvm);
        *err = jig_error_internal("Cannot get Window from Activity");
        return NULL;
    }

    /* Get DecorView dimensions */
    jmethodID getDecorView = (*env)->GetMethodID(env,
        (*env)->GetObjectClass(env, window), "getDecorView", "()Landroid/view/View;");
    jobject decorView = (*env)->CallObjectMethod(env, window, getDecorView);
    jmethodID viewGetWidth = (*env)->GetMethodID(env,
        (*env)->GetObjectClass(env, decorView), "getWidth", "()I");
    jmethodID viewGetHeight = (*env)->GetMethodID(env,
        (*env)->GetObjectClass(env, decorView), "getHeight", "()I");
    int width = (*env)->CallIntMethod(env, decorView, viewGetWidth);
    int height = (*env)->CallIntMethod(env, decorView, viewGetHeight);

    if (width <= 0 || height <= 0) {
        (*env)->DeleteLocalRef(env, activity);
        if (attached) (*jvm)->DetachCurrentThread(jvm);
        *err = jig_error_internal("DecorView has zero dimensions");
        return NULL;
    }

    /* Create Bitmap */
    jobject config = (*env)->GetStaticObjectField(env, s_bitmap_config_class, s_argb8888_field);
    jobject bitmap = (*env)->CallStaticObjectMethod(env, s_bitmap_class,
        s_bitmap_create, width, height, config);

    /* Capture via PixelCopy — reads from the HW-accelerated surface compositor.
     * JigScreenshotHelper.capture() wraps PixelCopy.request() with a
     * CountDownLatch, blocking until the copy completes (5s timeout).
     * Safe to block the main thread here — PixelCopy reads from SurfaceFlinger. */
    jclass helper_class = (*env)->FindClass(env, "jig/JigScreenshotHelper");
    if (!helper_class) {
        (*env)->CallVoidMethod(env, bitmap, s_bitmap_recycle);
        (*env)->DeleteLocalRef(env, activity);
        if (attached) (*jvm)->DetachCurrentThread(jvm);
        *err = jig_error_internal("JigScreenshotHelper class not found");
        return NULL;
    }
    jmethodID capture_method = (*env)->GetStaticMethodID(env, helper_class,
        "capture", "(Landroid/view/Window;Landroid/graphics/Bitmap;)I");
    if (!capture_method) {
        (*env)->CallVoidMethod(env, bitmap, s_bitmap_recycle);
        (*env)->DeleteLocalRef(env, activity);
        if (attached) (*jvm)->DetachCurrentThread(jvm);
        *err = jig_error_internal("JigScreenshotHelper.capture method not found");
        return NULL;
    }

    /* Retry PixelCopy up to 3 times — the surface may not be ready yet
     * on headless emulators (error code 3 = ERROR_SOURCE_NO_DATA). */
    jint copy_result = -1;
    for (int attempt = 0; attempt < 3; attempt++) {
        copy_result = (*env)->CallStaticIntMethod(env, helper_class,
            capture_method, window, bitmap);
        if (copy_result == 0) break;

        /* Sleep 500ms before retrying */
        struct timespec ts = { .tv_sec = 0, .tv_nsec = 500000000 };
        nanosleep(&ts, NULL);
    }

    if (copy_result != 0) {
        (*env)->CallVoidMethod(env, bitmap, s_bitmap_recycle);
        (*env)->DeleteLocalRef(env, activity);
        if (attached) (*jvm)->DetachCurrentThread(jvm);
        char msg[64];
        snprintf(msg, sizeof(msg), "PixelCopy failed with code %d after 3 attempts", (int)copy_result);
        *err = jig_error_internal(msg);
        return NULL;
    }

    /* Compress to JPEG or PNG */
    jobject compress_format;
    const char *actual_format;
    if (strcmp(format_str, "jpeg") == 0) {
        compress_format = (*env)->GetStaticObjectField(env, s_compress_format_class, s_jpeg_field);
        actual_format = "jpeg";
    } else {
        compress_format = (*env)->GetStaticObjectField(env, s_compress_format_class, s_png_field);
        actual_format = "png";
    }

    jobject baos = (*env)->NewObject(env, s_baos_class, s_baos_init);
    (*env)->CallBooleanMethod(env, bitmap, s_bitmap_compress,
        compress_format, quality, baos);

    /* Get byte array and base64 encode */
    jbyteArray bytes = (jbyteArray)(*env)->CallObjectMethod(env, baos, s_baos_toByteArray);
    jstring base64 = (jstring)(*env)->CallStaticObjectMethod(env, s_base64_class,
        s_base64_encodeToString, bytes, 2 /* NO_WRAP */);

    const char *base64_str = (*env)->GetStringUTFChars(env, base64, NULL);

    /* Get timestamp */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long long timestamp = (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

    /* Build cJSON result */
    cJSON *result = cJSON_CreateObject();
    cJSON_AddStringToObject(result, "data", base64_str);
    cJSON_AddStringToObject(result, "format", actual_format);
    cJSON_AddNumberToObject(result, "width", (double)width);
    cJSON_AddNumberToObject(result, "height", (double)height);
    cJSON_AddNumberToObject(result, "timestamp", (double)timestamp);

    /* Cleanup */
    (*env)->ReleaseStringUTFChars(env, base64, base64_str);
    (*env)->CallVoidMethod(env, bitmap, s_bitmap_recycle);
    (*env)->DeleteLocalRef(env, activity);
    if (attached) (*jvm)->DetachCurrentThread(jvm);

    return result;
}

jig_handler *jig_screenshot_handler_create(void) {
    jig_handler *h = calloc(1, sizeof(jig_handler));
    h->method = "jig.screenshot";
    h->thread_target = JIG_THREAD_MAIN;
    h->handle = screenshot_handle;
    return h;
}
