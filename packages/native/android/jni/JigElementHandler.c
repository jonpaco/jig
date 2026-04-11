#include "JigElementHandler.h"
#include "JigViewWalker.h"
#include "../../core/jig_selector.h"
#include "../../core/jig_suggestions.h"
#include "../../core/jig_errors.h"
#include "../../core/vendor/cJSON/cJSON.h"
#include <jni.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SUGGESTIONS 5

extern JavaVM *jig_get_jvm(void);

static cJSON *strip_internal_fields(cJSON *element) {
    cJSON *clean = cJSON_Duplicate(element, 1);
    cJSON_DeleteItemFromObject(clean, "parentReactTag");
    return clean;
}

static cJSON *build_visible_summary(cJSON *elements) {
    cJSON *summary = cJSON_CreateArray();
    cJSON *el = NULL;
    cJSON_ArrayForEach(el, elements) {
        cJSON *vis = cJSON_GetObjectItemCaseSensitive(el, "visible");
        if (!vis || !cJSON_IsTrue(vis)) continue;
        cJSON *entry = cJSON_CreateObject();
        cJSON *tid = cJSON_GetObjectItemCaseSensitive(el, "testID");
        if (tid && cJSON_IsString(tid)) cJSON_AddStringToObject(entry, "testID", tid->valuestring);
        cJSON_AddItemToArray(summary, entry);
    }
    return summary;
}

static cJSON *query_elements(void) {
    JavaVM *jvm = jig_get_jvm();
    JNIEnv *env;
    int attached = 0;
    jint rc = (*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_6);
    if (rc == JNI_EDETACHED) {
        (*jvm)->AttachCurrentThread(jvm, &env, NULL);
        attached = 1;
    }

    cJSON *elements = jig_android_walk_views(env);

    if (attached) {
        (*jvm)->DetachCurrentThread(jvm);
    }

    return elements;
}

static cJSON *find_element_handle(jig_handler *self, cJSON *params,
                                   jig_session *session, jig_error **err) {
    (void)self; (void)session;
    cJSON *selector = cJSON_GetObjectItemCaseSensitive(params, "selector");
    if (!selector) {
        *err = jig_error_invalid_params("Missing 'selector' field");
        return NULL;
    }
    cJSON *elements = query_elements();
    cJSON *match = jig_selector_find_one(elements, selector);
    if (!match) {
        cJSON *suggestions = jig_suggest_elements(elements, selector, MAX_SUGGESTIONS);
        cJSON *visible_summary = build_visible_summary(elements);
        cJSON *error_data = cJSON_CreateObject();
        cJSON_AddItemToObject(error_data, "selector", cJSON_Duplicate(selector, 1));
        cJSON *suggestion_strings = cJSON_CreateArray();
        cJSON *sug = NULL;
        cJSON_ArrayForEach(sug, suggestions) {
            cJSON *tid = cJSON_GetObjectItem(sug, "testID");
            if (tid && cJSON_IsString(tid))
                cJSON_AddItemToArray(suggestion_strings, cJSON_CreateString(tid->valuestring));
        }
        cJSON_AddItemToObject(error_data, "suggestions", suggestion_strings);
        cJSON_AddItemToObject(error_data, "visibleElements", visible_summary);
        cJSON_Delete(suggestions);
        *err = jig_error_element_not_found("No element matching selector", error_data);
        cJSON_Delete(elements);
        return NULL;
    }
    cJSON *clean = strip_internal_fields(match);
    cJSON_Delete(match);
    cJSON_Delete(elements);
    cJSON *result = cJSON_CreateObject();
    cJSON_AddItemToObject(result, "element", clean);
    return result;
}

static cJSON *find_elements_handle(jig_handler *self, cJSON *params,
                                    jig_session *session, jig_error **err) {
    (void)self; (void)session;
    cJSON *selector = cJSON_GetObjectItemCaseSensitive(params, "selector");
    if (!selector) {
        *err = jig_error_invalid_params("Missing 'selector' field");
        return NULL;
    }
    cJSON *elements = query_elements();
    cJSON *matches = jig_selector_find_all(elements, selector);
    cJSON_Delete(elements);
    cJSON *clean_matches = cJSON_CreateArray();
    cJSON *m = NULL;
    cJSON_ArrayForEach(m, matches) {
        cJSON_AddItemToArray(clean_matches, strip_internal_fields(m));
    }
    cJSON_Delete(matches);
    cJSON *result = cJSON_CreateObject();
    cJSON_AddItemToObject(result, "elements", clean_matches);
    return result;
}

jig_handler *jig_android_find_element_handler_create(void) {
    jig_handler *h = calloc(1, sizeof(jig_handler));
    h->method = "jig.findElement";
    h->thread_target = JIG_THREAD_MAIN;
    h->handle = find_element_handle;
    return h;
}

jig_handler *jig_android_find_elements_handler_create(void) {
    jig_handler *h = calloc(1, sizeof(jig_handler));
    h->method = "jig.findElements";
    h->thread_target = JIG_THREAD_MAIN;
    h->handle = find_elements_handle;
    return h;
}
