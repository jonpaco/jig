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

static const char *role_from_classname(const char *classname) {
    if (!classname) return NULL;
    if (strcmp(classname, "android.widget.Button") == 0) return "button";
    if (strcmp(classname, "android.widget.ImageButton") == 0) return "button";
    if (strcmp(classname, "android.widget.ImageView") == 0) return "image";
    if (strcmp(classname, "android.widget.SeekBar") == 0) return "adjustable";
    return NULL;
}

static void walk_view(JNIEnv *env, jobject view, int parent_tag,
                      cJSON *results, jclass view_class, jclass viewgroup_class,
                      jclass textview_class) {
    jmethodID getIdMethod = (*env)->GetMethodID(env, view_class, "getId", "()I");
    jint viewId = (*env)->CallIntMethod(env, view, getIdMethod);

    jmethodID getContentDescMethod = (*env)->GetMethodID(
        env, view_class, "getContentDescription", "()Ljava/lang/CharSequence;");
    jobject contentDesc = (*env)->CallObjectMethod(env, view, getContentDescMethod);
    char *contentDescStr = NULL;
    if (contentDesc) {
        jmethodID toStringMethod = (*env)->GetMethodID(
            env, (*env)->GetObjectClass(env, contentDesc), "toString",
            "()Ljava/lang/String;");
        jstring contentStr = (*env)->CallObjectMethod(env, contentDesc, toStringMethod);
        contentDescStr = jstring_to_cstr(env, contentStr);
        (*env)->DeleteLocalRef(env, contentStr);
        (*env)->DeleteLocalRef(env, contentDesc);
    }

    /* Extract text from TextView.getText() before the filter gate */
    char *textContent = NULL;
    if ((*env)->IsInstanceOf(env, view, textview_class)) {
        jmethodID getText = (*env)->GetMethodID(
            env, textview_class, "getText", "()Ljava/lang/CharSequence;");
        jobject textObj = (*env)->CallObjectMethod(env, view, getText);
        if (textObj) {
            jmethodID toString = (*env)->GetMethodID(
                env, (*env)->GetObjectClass(env, textObj), "toString",
                "()Ljava/lang/String;");
            jstring textStr = (*env)->CallObjectMethod(env, textObj, toString);
            textContent = jstring_to_cstr(env, textStr);
            if (textContent && strlen(textContent) == 0) {
                free(textContent);
                textContent = NULL;
            }
            (*env)->DeleteLocalRef(env, textStr);
            (*env)->DeleteLocalRef(env, textObj);
        }
    }

    if (viewId <= 0 && !contentDescStr && !textContent) {
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
        free(contentDescStr);
        free(textContent);
        return;
    }

    int tag = (int)viewId;
    cJSON *el = cJSON_CreateObject();
    cJSON_AddNumberToObject(el, "reactTag", tag);
    if (parent_tag >= 0) {
        cJSON_AddNumberToObject(el, "parentReactTag", parent_tag);
    }

    /* text = textContent ?? contentDescStr */
    if (textContent) {
        cJSON_AddStringToObject(el, "text", textContent);
        free(textContent);
    } else if (contentDescStr) {
        cJSON_AddStringToObject(el, "text", contentDescStr);
    }
    free(contentDescStr);

    /* --- testID: React Native sets testID via View.setTag() --- */
    jmethodID getTag = (*env)->GetMethodID(env, view_class, "getTag", "()Ljava/lang/Object;");
    jobject tagObj = (*env)->CallObjectMethod(env, view, getTag);
    if (tagObj) {
        jclass stringClass = (*env)->FindClass(env, "java/lang/String");
        if ((*env)->IsInstanceOf(env, tagObj, stringClass)) {
            char *tagStr = jstring_to_cstr(env, (jstring)tagObj);
            if (tagStr && strlen(tagStr) > 0) {
                cJSON_AddStringToObject(el, "testID", tagStr);
            }
            free(tagStr);
        }
        (*env)->DeleteLocalRef(env, stringClass);
        (*env)->DeleteLocalRef(env, tagObj);
    }

    /* --- Role extraction via accessibility API --- */
    {
        jmethodID createNodeInfo = (*env)->GetMethodID(
            env, view_class, "createAccessibilityNodeInfo",
            "()Landroid/view/accessibility/AccessibilityNodeInfo;");
        jobject nodeInfo = (*env)->CallObjectMethod(env, view, createNodeInfo);
        if (nodeInfo) {
            const char *role = NULL;
            jclass nodeClass = (*env)->GetObjectClass(env, nodeInfo);

            /* Try className mapping first */
            jmethodID getClassName = (*env)->GetMethodID(
                env, nodeClass, "getClassName", "()Ljava/lang/CharSequence;");
            jobject classNameObj = (*env)->CallObjectMethod(env, nodeInfo, getClassName);
            if (classNameObj) {
                jmethodID toString = (*env)->GetMethodID(
                    env, (*env)->GetObjectClass(env, classNameObj), "toString",
                    "()Ljava/lang/String;");
                jstring classNameStr = (*env)->CallObjectMethod(env, classNameObj, toString);
                char *className = jstring_to_cstr(env, classNameStr);
                role = role_from_classname(className);
                free(className);
                (*env)->DeleteLocalRef(env, classNameStr);
                (*env)->DeleteLocalRef(env, classNameObj);
            }

            /* Fallback: isHeading() for header role (API 28+) */
            if (!role) {
                jmethodID isHeading = (*env)->GetMethodID(
                    env, nodeClass, "isHeading", "()Z");
                if (!(*env)->ExceptionCheck(env)) {
                    jboolean heading = (*env)->CallBooleanMethod(env, nodeInfo, isHeading);
                    if (heading) role = "header";
                } else {
                    (*env)->ExceptionClear(env);
                }
            }

            /* Fallback: getRoleDescription() for link detection */
            if (!role) {
                jmethodID getRoleDesc = (*env)->GetMethodID(
                    env, nodeClass, "getRoleDescription", "()Ljava/lang/CharSequence;");
                if (!(*env)->ExceptionCheck(env)) {
                    jobject roleDescObj = (*env)->CallObjectMethod(env, nodeInfo, getRoleDesc);
                    if (roleDescObj) {
                        jmethodID toString = (*env)->GetMethodID(
                            env, (*env)->GetObjectClass(env, roleDescObj), "toString",
                            "()Ljava/lang/String;");
                        jstring roleDescStr = (*env)->CallObjectMethod(env, roleDescObj, toString);
                        char *roleDesc = jstring_to_cstr(env, roleDescStr);
                        if (roleDesc && strcmp(roleDesc, "Link") == 0) role = "link";
                        free(roleDesc);
                        (*env)->DeleteLocalRef(env, roleDescStr);
                        (*env)->DeleteLocalRef(env, roleDescObj);
                    }
                } else {
                    (*env)->ExceptionClear(env);
                }
            }

            if (role) {
                cJSON_AddStringToObject(el, "role", role);
            }

            /* Recycle to return to the pool */
            jmethodID recycle = (*env)->GetMethodID(env, nodeClass, "recycle", "()V");
            (*env)->CallVoidMethod(env, nodeInfo, recycle);
            (*env)->DeleteLocalRef(env, nodeClass);
            (*env)->DeleteLocalRef(env, nodeInfo);
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
