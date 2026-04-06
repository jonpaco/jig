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
    (void)elements; (void)selector;
    return NULL;
}

cJSON *jig_selector_find_all(const cJSON *elements, const cJSON *selector) {
    (void)elements; (void)selector;
    return cJSON_CreateArray();
}
