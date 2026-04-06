#ifndef JIG_VIEW_WALKER_ANDROID_H
#define JIG_VIEW_WALKER_ANDROID_H

#include "../../core/vendor/cJSON/cJSON.h"
#include <jni.h>

cJSON *jig_android_walk_views(JNIEnv *env);

#endif
