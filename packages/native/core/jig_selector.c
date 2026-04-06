#include "jig_selector.h"
#include <string.h>
#include <stdlib.h>

static bool match_string_field(const cJSON *element, const cJSON *selector, const char *field) {
    cJSON *sel_val = cJSON_GetObjectItemCaseSensitive(selector, field);
    if (!sel_val || !cJSON_IsString(sel_val)) return true;
    cJSON *el_val = cJSON_GetObjectItemCaseSensitive(element, field);
    if (!el_val || !cJSON_IsString(el_val)) return false;
    return strcmp(el_val->valuestring, sel_val->valuestring) == 0;
}

bool jig_selector_matches(const cJSON *element, const cJSON *selector) {
    if (!element || !selector) return false;
    if (!match_string_field(element, selector, "testID")) return false;
    if (!match_string_field(element, selector, "text")) return false;
    if (!match_string_field(element, selector, "role")) return false;
    if (!match_string_field(element, selector, "component")) return false;
    return true;
}

cJSON *jig_selector_find_one(const cJSON *elements, const cJSON *selector) {
    if (!elements || !selector) return NULL;

    cJSON *within = cJSON_GetObjectItemCaseSensitive(selector, "within");
    cJSON *inner = cJSON_GetObjectItemCaseSensitive(selector, "selector");
    if (within && inner) {
        return NULL; /* Placeholder for within — Task 4 */
    }

    cJSON *idx_item = cJSON_GetObjectItemCaseSensitive(selector, "index");
    int target_idx = -1;
    if (idx_item && cJSON_IsNumber(idx_item)) {
        target_idx = idx_item->valueint;
    }

    int match_count = 0;
    cJSON *el = NULL;
    cJSON_ArrayForEach(el, elements) {
        if (jig_selector_matches(el, selector)) {
            if (target_idx < 0) return cJSON_Duplicate(el, 1);
            if (match_count == target_idx) return cJSON_Duplicate(el, 1);
            match_count++;
        }
    }
    return NULL;
}

cJSON *jig_selector_find_all(const cJSON *elements, const cJSON *selector) {
    cJSON *result = cJSON_CreateArray();
    if (!elements || !selector) return result;

    cJSON *within = cJSON_GetObjectItemCaseSensitive(selector, "within");
    cJSON *inner = cJSON_GetObjectItemCaseSensitive(selector, "selector");
    if (within && inner) {
        return result; /* Placeholder for within — Task 4 */
    }

    cJSON *el = NULL;
    cJSON_ArrayForEach(el, elements) {
        if (jig_selector_matches(el, selector)) {
            cJSON_AddItemToArray(result, cJSON_Duplicate(el, 1));
        }
    }
    return result;
}
