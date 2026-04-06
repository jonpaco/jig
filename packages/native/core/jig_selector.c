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

static bool is_descendant_of(const cJSON *elements, const cJSON *element, int ancestor_tag) {
    cJSON *parent_ref = cJSON_GetObjectItemCaseSensitive(element, "parentReactTag");
    if (!parent_ref || !cJSON_IsNumber(parent_ref)) return false;
    int parent_tag = parent_ref->valueint;
    if (parent_tag == ancestor_tag) return true;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, elements) {
        cJSON *tag = cJSON_GetObjectItemCaseSensitive(item, "reactTag");
        if (tag && cJSON_IsNumber(tag) && tag->valueint == parent_tag) {
            return is_descendant_of(elements, item, ancestor_tag);
        }
    }
    return false;
}

static cJSON *get_descendants(const cJSON *elements, int container_tag) {
    cJSON *descendants = cJSON_CreateArray();
    cJSON *el = NULL;
    cJSON_ArrayForEach(el, elements) {
        if (is_descendant_of(elements, el, container_tag)) {
            cJSON_AddItemToArray(descendants, cJSON_Duplicate(el, 1));
        }
    }
    return descendants;
}

cJSON *jig_selector_find_one(const cJSON *elements, const cJSON *selector) {
    if (!elements || !selector) return NULL;

    cJSON *within = cJSON_GetObjectItemCaseSensitive(selector, "within");
    cJSON *inner = cJSON_GetObjectItemCaseSensitive(selector, "selector");
    if (within && inner) {
        cJSON *container = jig_selector_find_one(elements, within);
        if (!container) return NULL;
        cJSON *tag = cJSON_GetObjectItemCaseSensitive(container, "reactTag");
        int container_tag = tag ? tag->valueint : -1;
        cJSON_Delete(container);
        if (container_tag < 0) return NULL;
        cJSON *descendants = get_descendants(elements, container_tag);
        cJSON *result = jig_selector_find_one(descendants, inner);
        cJSON_Delete(descendants);
        return result;
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
        cJSON *container = jig_selector_find_one(elements, within);
        if (!container) return result;
        cJSON *tag = cJSON_GetObjectItemCaseSensitive(container, "reactTag");
        int container_tag = tag ? tag->valueint : -1;
        cJSON_Delete(container);
        if (container_tag < 0) return result;
        cJSON *descendants = get_descendants(elements, container_tag);
        cJSON_Delete(result);
        result = jig_selector_find_all(descendants, inner);
        cJSON_Delete(descendants);
        return result;
    }

    cJSON *el = NULL;
    cJSON_ArrayForEach(el, elements) {
        if (jig_selector_matches(el, selector)) {
            cJSON_AddItemToArray(result, cJSON_Duplicate(el, 1));
        }
    }
    return result;
}
