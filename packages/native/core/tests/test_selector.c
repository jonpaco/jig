#include "test_util.h"
#include "../jig_selector.h"
#include <string.h>

static cJSON *make_element(int tag, const char *testID, const char *text,
                           const char *role, const char *component,
                           int parent_tag) {
    cJSON *el = cJSON_CreateObject();
    cJSON_AddNumberToObject(el, "reactTag", tag);
    if (parent_tag >= 0) cJSON_AddNumberToObject(el, "parentReactTag", parent_tag);
    if (testID) cJSON_AddStringToObject(el, "testID", testID);
    if (text) cJSON_AddStringToObject(el, "text", text);
    if (role) cJSON_AddStringToObject(el, "role", role);
    if (component) cJSON_AddStringToObject(el, "component", component);
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
    cJSON *el = make_element(1, "habit-card", NULL, NULL, NULL, -1);
    cJSON *sel = make_selector_str("testID", "habit-card");
    ASSERT(jig_selector_matches(el, sel) == 1, "testID: exact match");
    cJSON_Delete(sel);
    sel = make_selector_str("testID", "other-card");
    ASSERT(jig_selector_matches(el, sel) == 0, "testID: mismatch");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_match_text(void) {
    cJSON *el = make_element(1, NULL, "Running", NULL, NULL, -1);
    cJSON *sel = make_selector_str("text", "Running");
    ASSERT(jig_selector_matches(el, sel) == 1, "text: exact match");
    cJSON_Delete(sel);
    sel = make_selector_str("text", "Walking");
    ASSERT(jig_selector_matches(el, sel) == 0, "text: mismatch");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_match_role(void) {
    cJSON *el = make_element(1, NULL, NULL, "button", NULL, -1);
    cJSON *sel = make_selector_str("role", "button");
    ASSERT(jig_selector_matches(el, sel) == 1, "role: exact match");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_match_component(void) {
    cJSON *el = make_element(1, NULL, NULL, NULL, "HabitCard", -1);
    cJSON *sel = make_selector_str("component", "HabitCard");
    ASSERT(jig_selector_matches(el, sel) == 1, "component: exact match");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_match_missing_field(void) {
    cJSON *el = make_element(1, NULL, NULL, NULL, NULL, -1);
    cJSON *sel = make_selector_str("testID", "anything");
    ASSERT(jig_selector_matches(el, sel) == 0, "missing field: no match");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_match_and_fields(void) {
    cJSON *el = make_element(1, NULL, "Save", "button", NULL, -1);
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
    cJSON *el = make_element(1, "card", "Hello", "text", "MyComp", -1);
    cJSON *sel = cJSON_CreateObject();
    ASSERT(jig_selector_matches(el, sel) == 1, "empty selector: matches any element");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_find_one_basic(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_element(10, "header", NULL, NULL, NULL, -1));
    cJSON_AddItemToArray(elements, make_element(20, "habit-card", NULL, NULL, NULL, 10));
    cJSON_AddItemToArray(elements, make_element(30, "footer", NULL, NULL, NULL, -1));
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
    cJSON_AddItemToArray(elements, make_element(10, "header", NULL, NULL, NULL, -1));
    cJSON *sel = make_selector_str("testID", "nonexistent");
    cJSON *result = jig_selector_find_one(elements, sel);
    ASSERT(result == NULL, "find_one no match: returns NULL");
    cJSON_Delete(sel);
    cJSON_Delete(elements);
}

static void test_find_all_basic(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_element(10, NULL, NULL, NULL, "HabitCard", -1));
    cJSON_AddItemToArray(elements, make_element(20, NULL, NULL, NULL, "Header", -1));
    cJSON_AddItemToArray(elements, make_element(30, NULL, NULL, NULL, "HabitCard", -1));
    cJSON *sel = make_selector_str("component", "HabitCard");
    cJSON *result = jig_selector_find_all(elements, sel);
    ASSERT(cJSON_GetArraySize(result) == 2, "find_all basic: found 2 elements");
    cJSON_Delete(result);
    cJSON_Delete(sel);
    cJSON_Delete(elements);
}

static void test_find_all_empty(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_element(10, "card", NULL, NULL, NULL, -1));
    cJSON *sel = make_selector_str("testID", "nonexistent");
    cJSON *result = jig_selector_find_all(elements, sel);
    ASSERT(cJSON_GetArraySize(result) == 0, "find_all empty: returns empty array");
    cJSON_Delete(result);
    cJSON_Delete(sel);
    cJSON_Delete(elements);
}

static void test_find_one_with_index(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_element(10, NULL, NULL, NULL, "HabitCard", -1));
    cJSON_AddItemToArray(elements, make_element(20, NULL, NULL, NULL, "HabitCard", -1));
    cJSON_AddItemToArray(elements, make_element(30, NULL, NULL, NULL, "HabitCard", -1));
    cJSON *sel = cJSON_CreateObject();
    cJSON_AddStringToObject(sel, "component", "HabitCard");
    cJSON_AddNumberToObject(sel, "index", 2);
    cJSON *result = jig_selector_find_one(elements, sel);
    ASSERT(result != NULL, "find_one index: found element");
    cJSON *tag = cJSON_GetObjectItem(result, "reactTag");
    ASSERT(tag && tag->valueint == 30, "find_one index: correct reactTag (index 2)");
    cJSON_Delete(result);
    cJSON_Delete(sel);
    sel = cJSON_CreateObject();
    cJSON_AddStringToObject(sel, "component", "HabitCard");
    cJSON_AddNumberToObject(sel, "index", 5);
    result = jig_selector_find_one(elements, sel);
    ASSERT(result == NULL, "find_one index out of range: returns NULL");
    cJSON_Delete(sel);
    cJSON_Delete(elements);
}

static void test_find_all_with_empty_selector(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_element(10, "a", NULL, NULL, NULL, -1));
    cJSON_AddItemToArray(elements, make_element(20, "b", NULL, NULL, NULL, -1));
    cJSON *sel = cJSON_CreateObject();
    cJSON *result = jig_selector_find_all(elements, sel);
    ASSERT(cJSON_GetArraySize(result) == 2, "find_all empty selector: returns all elements");
    cJSON_Delete(result);
    cJSON_Delete(sel);
    cJSON_Delete(elements);
}

int main(void) {
    /* Task 2: basic field matching */
    test_match_testID();
    test_match_text();
    test_match_role();
    test_match_component();
    test_match_missing_field();
    test_match_and_fields();
    test_empty_selector_matches_all();

    /* Task 3: find_one, find_all, index */
    test_find_one_basic();
    test_find_one_no_match();
    test_find_all_basic();
    test_find_all_empty();
    test_find_one_with_index();
    test_find_all_with_empty_selector();

    if (failures == 0) printf("test_selector: all tests passed\n");
    else printf("test_selector: %d failure(s)\n", failures);
    return failures;
}
