#include "JigViewWalker.h"
#include <android/log.h>
#include <string.h>
#include <stdlib.h>

#define LOG_TAG "Jig"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern jobject jig_get_current_activity(JNIEnv *env);

static char *jstring_to_cstr(JNIEnv *env, jstring jstr) {
    if (!jstr) return NULL;
    const char *utf = (*env)->GetStringUTFChars(env, jstr, NULL);
    if (!utf) return NULL;
    char *copy = strdup(utf);
    (*env)->ReleaseStringUTFChars(env, jstr, utf);
    return copy;
}

static void walk_view(JNIEnv *env, jobject view, int parent_tag,
                      cJSON *results, jclass view_class, jclass viewgroup_class,
                      jclass textview_class) {
    jmethodID getIdMethod = (*env)->GetMethodID(env, view_class, "getId", "()I");
    jint viewId = (*env)->CallIntMethod(env, view, getIdMethod);

    jmethodID getContentDescMethod = (*env)->GetMethodID(
        env, view_class, "getContentDescription", "()Ljava/lang/CharSequence;");
    jobject contentDesc = (*env)->CallObjectMethod(env, view, getContentDescMethod);
    char *testID = NULL;
    if (contentDesc) {
        jmethodID toStringMethod = (*env)->GetMethodID(
            env, (*env)->GetObjectClass(env, contentDesc), "toString",
            "()Ljava/lang/String;");
        jstring contentStr = (*env)->CallObjectMethod(env, contentDesc, toStringMethod);
        testID = jstring_to_cstr(env, contentStr);
        (*env)->DeleteLocalRef(env, contentStr);
        (*env)->DeleteLocalRef(env, contentDesc);
    }

    if (viewId <= 0 && !testID) {
        if ((*env)->IsInstanceOf(env, view, viewgroup_class)) {
            jmethodID getChildCount = (*env)->GetMethodID(
                env, viewgroup_class, "getChildCount", "()I");
            jmethodID getChildAt = (*env)->GetMethodID(
                env, viewgroup_class, "getChildAt", "(I)Landroid/view/View;");
            jint count = (*env)->CallIntMethod(env, view, getChildCount);
            for (jint i = 0; i < count; i++) {
                jobject child = (*env)->CallObjectMethod(env, view, getChildAt, i);
                if (child) {
                    walk_view(env, child, parent_tag, results,
                              view_class, viewgroup_class, textview_class);
                    (*env)->DeleteLocalRef(env, child);
                }
            }
        }
        free(testID);
        return;
    }

    int tag = (int)viewId;
    cJSON *el = cJSON_CreateObject();
    cJSON_AddNumberToObject(el, "reactTag", tag);
    if (parent_tag >= 0) {
        cJSON_AddNumberToObject(el, "parentReactTag", parent_tag);
    }

    if (testID) {
        cJSON_AddStringToObject(el, "testID", testID);
        free(testID);
    }

    if ((*env)->IsInstanceOf(env, view, textview_class)) {
        jmethodID getText = (*env)->GetMethodID(
            env, textview_class, "getText", "()Ljava/lang/CharSequence;");
        jobject textObj = (*env)->CallObjectMethod(env, view, getText);
        if (textObj) {
            jmethodID toString = (*env)->GetMethodID(
                env, (*env)->GetObjectClass(env, textObj), "toString",
                "()Ljava/lang/String;");
            jstring textStr = (*env)->CallObjectMethod(env, textObj, toString);
            char *text = jstring_to_cstr(env, textStr);
            if (text && strlen(text) > 0) {
                cJSON_AddStringToObject(el, "text", text);
            }
            free(text);
            (*env)->DeleteLocalRef(env, textStr);
            (*env)->DeleteLocalRef(env, textObj);
        }
    }

    jintArray locArray = (*env)->NewIntArray(env, 2);
    jmethodID getLocation = (*env)->GetMethodID(
        env, view_class, "getLocationOnScreen", "([I)V");
    (*env)->CallVoidMethod(env, view, getLocation, locArray);
    jint *loc = (*env)->GetIntArrayElements(env, locArray, NULL);

    jmethodID getWidth = (*env)->GetMethodID(env, view_class, "getWidth", "()I");
    jmethodID getHeight = (*env)->GetMethodID(env, view_class, "getHeight", "()I");
    jint width = (*env)->CallIntMethod(env, view, getWidth);
    jint height = (*env)->CallIntMethod(env, view, getHeight);

    cJSON *frame = cJSON_CreateObject();
    cJSON_AddNumberToObject(frame, "x", loc[0]);
    cJSON_AddNumberToObject(frame, "y", loc[1]);
    cJSON_AddNumberToObject(frame, "width", width);
    cJSON_AddNumberToObject(frame, "height", height);
    cJSON_AddItemToObject(el, "frame", frame);

    (*env)->ReleaseIntArrayElements(env, locArray, loc, 0);
    (*env)->DeleteLocalRef(env, locArray);

    jmethodID getVisibility = (*env)->GetMethodID(
        env, view_class, "getVisibility", "()I");
    jint visibility = (*env)->CallIntMethod(env, view, getVisibility);

    jclass rectClass = (*env)->FindClass(env, "android/graphics/Rect");
    jmethodID rectInit = (*env)->GetMethodID(env, rectClass, "<init>", "()V");
    jobject rect = (*env)->NewObject(env, rectClass, rectInit);
    jmethodID getGlobalVisible = (*env)->GetMethodID(
        env, view_class, "getGlobalVisibleRect", "(Landroid/graphics/Rect;)Z");
    jboolean globalVisible = (*env)->CallBooleanMethod(
        env, view, getGlobalVisible, rect);
    (*env)->DeleteLocalRef(env, rect);
    (*env)->DeleteLocalRef(env, rectClass);

    int is_visible = (visibility == 0 && globalVisible);
    cJSON_AddBoolToObject(el, "visible", is_visible);

    cJSON_AddItemToArray(results, el);

    if ((*env)->IsInstanceOf(env, view, viewgroup_class)) {
        jmethodID getChildCount = (*env)->GetMethodID(
            env, viewgroup_class, "getChildCount", "()I");
        jmethodID getChildAt = (*env)->GetMethodID(
            env, viewgroup_class, "getChildAt", "(I)Landroid/view/View;");
        jint count = (*env)->CallIntMethod(env, view, getChildCount);
        for (jint i = 0; i < count; i++) {
            jobject child = (*env)->CallObjectMethod(env, view, getChildAt, i);
            if (child) {
                walk_view(env, child, tag, results,
                          view_class, viewgroup_class, textview_class);
                (*env)->DeleteLocalRef(env, child);
            }
        }
    }
}

cJSON *jig_android_walk_views(JNIEnv *env) {
    cJSON *results = cJSON_CreateArray();

    jobject activity = jig_get_current_activity(env);
    if (!activity) {
        LOGI("walk_views: no current activity");
        return results;
    }

    jclass activityClass = (*env)->GetObjectClass(env, activity);
    jmethodID getWindow = (*env)->GetMethodID(
        env, activityClass, "getWindow", "()Landroid/view/Window;");
    jobject window = (*env)->CallObjectMethod(env, activity, getWindow);
    if (!window) {
        (*env)->DeleteLocalRef(env, activity);
        (*env)->DeleteLocalRef(env, activityClass);
        return results;
    }

    jclass windowClass = (*env)->GetObjectClass(env, window);
    jmethodID getDecorView = (*env)->GetMethodID(
        env, windowClass, "getDecorView", "()Landroid/view/View;");
    jobject decorView = (*env)->CallObjectMethod(env, window, getDecorView);
    if (!decorView) {
        (*env)->DeleteLocalRef(env, window);
        (*env)->DeleteLocalRef(env, windowClass);
        (*env)->DeleteLocalRef(env, activity);
        (*env)->DeleteLocalRef(env, activityClass);
        return results;
    }

    jclass view_class = (*env)->FindClass(env, "android/view/View");
    jclass viewgroup_class = (*env)->FindClass(env, "android/view/ViewGroup");
    jclass textview_class = (*env)->FindClass(env, "android/widget/TextView");

    walk_view(env, decorView, -1, results,
              view_class, viewgroup_class, textview_class);

    (*env)->DeleteLocalRef(env, textview_class);
    (*env)->DeleteLocalRef(env, viewgroup_class);
    (*env)->DeleteLocalRef(env, view_class);
    (*env)->DeleteLocalRef(env, decorView);
    (*env)->DeleteLocalRef(env, window);
    (*env)->DeleteLocalRef(env, windowClass);
    (*env)->DeleteLocalRef(env, activity);
    (*env)->DeleteLocalRef(env, activityClass);

    return results;
}
