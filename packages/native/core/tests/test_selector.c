#include "test_util.h"
#include "../jig_selector.h"
#include <string.h>

static cJSON *make_element(int tag, const char *testID, const char *text,
                           const char *role, int parent_tag) {
    cJSON *el = cJSON_CreateObject();
    cJSON_AddNumberToObject(el, "reactTag", tag);
    if (parent_tag >= 0) cJSON_AddNumberToObject(el, "parentReactTag", parent_tag);
    if (testID) cJSON_AddStringToObject(el, "testID", testID);
    if (text) cJSON_AddStringToObject(el, "text", text);
    if (role) cJSON_AddStringToObject(el, "role", role);
    cJSON_AddBoolToObject(el, "visible", 1);
    cJSON *frame = cJSON_CreateObject();
    cJSON_AddNumberToObject(frame, "x", 0);
    cJSON_AddNumberToObject(frame, "y", 0);
    cJSON_AddNumberToObject(frame, "width", 100);
    cJSON_AddNumberToObject(frame, "height", 50);
    cJSON_AddItemToObject(el, "frame", frame);
    return el;
}

static cJSON *make_selector_str(const char *field, const char *value) {
    cJSON *sel = cJSON_CreateObject();
    cJSON_AddStringToObject(sel, field, value);
    return sel;
}

static void test_match_testID(void) {
    cJSON *el = make_element(1, "habit-card", NULL, NULL, -1);
    cJSON *sel = make_selector_str("testID", "habit-card");
    ASSERT(jig_selector_matches(el, sel) == 1, "testID: exact match");
    cJSON_Delete(sel);
    sel = make_selector_str("testID", "other-card");
    ASSERT(jig_selector_matches(el, sel) == 0, "testID: mismatch");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_match_text(void) {
    cJSON *el = make_element(1, NULL, "Running", NULL, -1);
    cJSON *sel = make_selector_str("text", "Running");
    ASSERT(jig_selector_matches(el, sel) == 1, "text: exact match");
    cJSON_Delete(sel);
    sel = make_selector_str("text", "Walking");
    ASSERT(jig_selector_matches(el, sel) == 0, "text: mismatch");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_match_role(void) {
    cJSON *el = make_element(1, NULL, NULL, "button", -1);
    cJSON *sel = make_selector_str("role", "button");
    ASSERT(jig_selector_matches(el, sel) == 1, "role: exact match");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_match_missing_field(void) {
    cJSON *el = make_element(1, NULL, NULL, NULL, -1);
    cJSON *sel = make_selector_str("testID", "anything");
    ASSERT(jig_selector_matches(el, sel) == 0, "missing field: no match");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_match_and_fields(void) {
    cJSON *el = make_element(1, NULL, "Save", "button", -1);
    cJSON *sel = cJSON_CreateObject();
    cJSON_AddStringToObject(sel, "role", "button");
    cJSON_AddStringToObject(sel, "text", "Save");
    ASSERT(jig_selector_matches(el, sel) == 1, "AND: both match");
    cJSON_Delete(sel);
    sel = cJSON_CreateObject();
    cJSON_AddStringToObject(sel, "role", "button");
    cJSON_AddStringToObject(sel, "text", "Cancel");
    ASSERT(jig_selector_matches(el, sel) == 0, "AND: one mismatch");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_empty_selector_matches_all(void) {
    cJSON *el = make_element(1, "card", "Hello", "text", -1);
    cJSON *sel = cJSON_CreateObject();
    ASSERT(jig_selector_matches(el, sel) == 1, "empty selector: matches any element");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_find_one_basic(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_element(10, "header", NULL, NULL, -1));
    cJSON_AddItemToArray(elements, make_element(20, "habit-card", NULL, NULL, 10));
    cJSON_AddItemToArray(elements, make_element(30, "footer", NULL, NULL, -1));
    cJSON *sel = make_selector_str("testID", "habit-card");
    cJSON *result = jig_selector_find_one(elements, sel);
    ASSERT(result != NULL, "find_one basic: found element");
    cJSON *tag = cJSON_GetObjectItem(result, "reactTag");
    ASSERT(tag && tag->valueint == 20, "find_one basic: correct reactTag");
    cJSON_Delete(result);
    cJSON_Delete(sel);
    cJSON_Delete(elements);
}

static void test_find_one_no_match(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_element(10, "header", NULL, NULL, -1));
    cJSON *sel = make_selector_str("testID", "nonexistent");
    cJSON *result = jig_selector_find_one(elements, sel);
    ASSERT(result == NULL, "find_one no match: returns NULL");
    cJSON_Delete(sel);
    cJSON_Delete(elements);
}

static void test_find_all_basic(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_element(10, NULL, NULL, "button", -1));
    cJSON_AddItemToArray(elements, make_element(20, NULL, NULL, "text", -1));
    cJSON_AddItemToArray(elements, make_element(30, NULL, NULL, "button", -1));
    cJSON *sel = make_selector_str("role", "button");
    cJSON *result = jig_selector_find_all(elements, sel);
    ASSERT(cJSON_GetArraySize(result) == 2, "find_all basic: found 2 buttons");
    cJSON_Delete(result);
    cJSON_Delete(sel);
    cJSON_Delete(elements);
}

static void test_find_all_empty(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_element(10, "card", NULL, NULL, -1));
    cJSON *sel = make_selector_str("testID", "nonexistent");
    cJSON *result = jig_selector_find_all(elements, sel);
    ASSERT(cJSON_GetArraySize(result) == 0, "find_all empty: returns empty array");
    cJSON_Delete(result);
    cJSON_Delete(sel);
    cJSON_Delete(elements);
}

static void test_find_one_with_index(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_element(10, NULL, NULL, "button", -1));
    cJSON_AddItemToArray(elements, make_element(20, NULL, NULL, "button", -1));
    cJSON_AddItemToArray(elements, make_element(30, NULL, NULL, "button", -1));
    cJSON *sel = cJSON_CreateObject();
    cJSON_AddStringToObject(sel, "role", "button");
    cJSON_AddNumberToObject(sel, "index", 2);
    cJSON *result = jig_selector_find_one(elements, sel);
    ASSERT(result != NULL, "find_one index: found element");
    cJSON *tag = cJSON_GetObjectItem(result, "reactTag");
    ASSERT(tag && tag->valueint == 30, "find_one index: correct reactTag (index 2)");
    cJSON_Delete(result);
    cJSON_Delete(sel);
    sel = cJSON_CreateObject();
    cJSON_AddStringToObject(sel, "role", "button");
    cJSON_AddNumberToObject(sel, "index", 5);
    result = jig_selector_find_one(elements, sel);
    ASSERT(result == NULL, "find_one index out of range: returns NULL");
    cJSON_Delete(sel);
    cJSON_Delete(elements);
}

static void test_find_all_with_empty_selector(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_element(10, "a", NULL, NULL, -1));
    cJSON_AddItemToArray(elements, make_element(20, "b", NULL, NULL, -1));
    cJSON *sel = cJSON_CreateObject();
    cJSON *result = jig_selector_find_all(elements, sel);
    ASSERT(cJSON_GetArraySize(result) == 2, "find_all empty selector: returns all elements");
    cJSON_Delete(result);
    cJSON_Delete(sel);
    cJSON_Delete(elements);
}

static void test_within_basic(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_element(10, "habits-list", NULL, NULL, -1));
    cJSON_AddItemToArray(elements, make_element(20, "card-a", NULL, NULL, 10));
    cJSON_AddItemToArray(elements, make_element(30, "card-b", NULL, NULL, 10));
    cJSON_AddItemToArray(elements, make_element(40, "settings", NULL, NULL, -1));
    cJSON_AddItemToArray(elements, make_element(50, "card-c", NULL, NULL, 40));

    cJSON *sel = cJSON_CreateObject();
    cJSON *within_sel = cJSON_CreateObject();
    cJSON_AddStringToObject(within_sel, "testID", "habits-list");
    cJSON_AddItemToObject(sel, "within", within_sel);
    cJSON *inner_sel = cJSON_CreateObject();
    cJSON_AddItemToObject(sel, "selector", inner_sel);

    cJSON *result = jig_selector_find_all(elements, sel);
    ASSERT(cJSON_GetArraySize(result) == 2, "within basic: found 2 elements inside habits-list");
    cJSON_Delete(result);

    cJSON *one = jig_selector_find_one(elements, sel);
    ASSERT(one != NULL, "within find_one: found element");
    cJSON *tag = cJSON_GetObjectItem(one, "reactTag");
    ASSERT(tag && tag->valueint == 20, "within find_one: first match is tag 20");
    cJSON_Delete(one);
    cJSON_Delete(sel);
    cJSON_Delete(elements);
}

static void test_within_with_index(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_element(10, "list", NULL, NULL, -1));
    cJSON_AddItemToArray(elements, make_element(20, "item-a", NULL, NULL, 10));
    cJSON_AddItemToArray(elements, make_element(30, "item-b", NULL, NULL, 10));
    cJSON_AddItemToArray(elements, make_element(40, "item-c", NULL, NULL, 10));

    cJSON *sel = cJSON_CreateObject();
    cJSON *within_sel = cJSON_CreateObject();
    cJSON_AddStringToObject(within_sel, "testID", "list");
    cJSON_AddItemToObject(sel, "within", within_sel);
    cJSON *inner_sel = cJSON_CreateObject();
    cJSON_AddNumberToObject(inner_sel, "index", 1);
    cJSON_AddItemToObject(sel, "selector", inner_sel);

    cJSON *result = jig_selector_find_one(elements, sel);
    ASSERT(result != NULL, "within+index: found element");
    cJSON *tag = cJSON_GetObjectItem(result, "reactTag");
    ASSERT(tag && tag->valueint == 30, "within+index: correct tag (index 1 = tag 30)");
    cJSON_Delete(result);
    cJSON_Delete(sel);
    cJSON_Delete(elements);
}

static void test_within_no_container(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_element(10, "card", NULL, NULL, -1));

    cJSON *sel = cJSON_CreateObject();
    cJSON *within_sel = cJSON_CreateObject();
    cJSON_AddStringToObject(within_sel, "testID", "nonexistent");
    cJSON_AddItemToObject(sel, "within", within_sel);
    cJSON *inner_sel = cJSON_CreateObject();
    cJSON_AddStringToObject(inner_sel, "testID", "card");
    cJSON_AddItemToObject(sel, "selector", inner_sel);

    cJSON *result = jig_selector_find_one(elements, sel);
    ASSERT(result == NULL, "within no container: returns NULL");
    cJSON *all = jig_selector_find_all(elements, sel);
    ASSERT(cJSON_GetArraySize(all) == 0, "within no container: returns empty array");
    cJSON_Delete(all);
    cJSON_Delete(sel);
    cJSON_Delete(elements);
}

static void test_within_deep_nesting(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_element(1, "root", NULL, NULL, -1));
    cJSON_AddItemToArray(elements, make_element(2, NULL, NULL, NULL, 1));
    cJSON_AddItemToArray(elements, make_element(3, "deep-child", NULL, NULL, 2));

    cJSON *sel = cJSON_CreateObject();
    cJSON *within_sel = cJSON_CreateObject();
    cJSON_AddStringToObject(within_sel, "testID", "root");
    cJSON_AddItemToObject(sel, "within", within_sel);
    cJSON *inner_sel = cJSON_CreateObject();
    cJSON_AddStringToObject(inner_sel, "testID", "deep-child");
    cJSON_AddItemToObject(sel, "selector", inner_sel);

    cJSON *result = jig_selector_find_one(elements, sel);
    ASSERT(result != NULL, "within deep: found deeply nested element");
    cJSON *tag = cJSON_GetObjectItem(result, "reactTag");
    ASSERT(tag && tag->valueint == 3, "within deep: correct tag");
    cJSON_Delete(result);
    cJSON_Delete(sel);
    cJSON_Delete(elements);
}

int main(void) {
    test_match_testID();
    test_match_text();
    test_match_role();
    test_match_missing_field();
    test_match_and_fields();
    test_empty_selector_matches_all();

    test_find_one_basic();
    test_find_one_no_match();
    test_find_all_basic();
    test_find_all_empty();
    test_find_one_with_index();
    test_find_all_with_empty_selector();

    test_within_basic();
    test_within_with_index();
    test_within_no_container();
    test_within_deep_nesting();

    if (failures == 0) printf("test_selector: all tests passed\n");
    else printf("test_selector: %d failure(s)\n", failures);
    return failures;
}
