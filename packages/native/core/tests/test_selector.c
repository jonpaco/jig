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

static cJSON *make_element_with_props(int tag, const char *component,
                                       cJSON *props, int parent_tag) {
    cJSON *el = make_element(tag, NULL, NULL, NULL, component, parent_tag);
    if (props) cJSON_AddItemToObject(el, "props", props);
    return el;
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

static void test_within_basic(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_element(10, "habits-list", NULL, NULL, NULL, -1));
    cJSON_AddItemToArray(elements, make_element(20, NULL, NULL, NULL, "HabitCard", 10));
    cJSON_AddItemToArray(elements, make_element(30, NULL, NULL, NULL, "HabitCard", 10));
    cJSON_AddItemToArray(elements, make_element(40, "settings", NULL, NULL, NULL, -1));
    cJSON_AddItemToArray(elements, make_element(50, NULL, NULL, NULL, "HabitCard", 40));

    cJSON *sel = cJSON_CreateObject();
    cJSON *within_sel = cJSON_CreateObject();
    cJSON_AddStringToObject(within_sel, "testID", "habits-list");
    cJSON_AddItemToObject(sel, "within", within_sel);
    cJSON *inner_sel = cJSON_CreateObject();
    cJSON_AddStringToObject(inner_sel, "component", "HabitCard");
    cJSON_AddItemToObject(sel, "selector", inner_sel);

    cJSON *result = jig_selector_find_all(elements, sel);
    ASSERT(cJSON_GetArraySize(result) == 2, "within basic: found 2 HabitCards inside habits-list");
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
    cJSON_AddItemToArray(elements, make_element(10, "list", NULL, NULL, NULL, -1));
    cJSON_AddItemToArray(elements, make_element(20, NULL, NULL, NULL, "Card", 10));
    cJSON_AddItemToArray(elements, make_element(30, NULL, NULL, NULL, "Card", 10));
    cJSON_AddItemToArray(elements, make_element(40, NULL, NULL, NULL, "Card", 10));

    cJSON *sel = cJSON_CreateObject();
    cJSON *within_sel = cJSON_CreateObject();
    cJSON_AddStringToObject(within_sel, "testID", "list");
    cJSON_AddItemToObject(sel, "within", within_sel);
    cJSON *inner_sel = cJSON_CreateObject();
    cJSON_AddStringToObject(inner_sel, "component", "Card");
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
    cJSON_AddItemToArray(elements, make_element(10, "card", NULL, NULL, NULL, -1));

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
    cJSON_AddItemToArray(elements, make_element(1, "root", NULL, NULL, NULL, -1));
    cJSON_AddItemToArray(elements, make_element(2, NULL, NULL, NULL, NULL, 1));
    cJSON_AddItemToArray(elements, make_element(3, "deep-child", NULL, NULL, NULL, 2));

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

static void test_match_props_string(void) {
    cJSON *props = cJSON_CreateObject();
    cJSON_AddStringToObject(props, "title", "Exercise");
    cJSON_AddStringToObject(props, "subtitle", "Daily");
    cJSON *el = make_element_with_props(1, "HabitCard", props, -1);

    cJSON *sel = cJSON_CreateObject();
    cJSON_AddStringToObject(sel, "component", "HabitCard");
    cJSON *sel_props = cJSON_CreateObject();
    cJSON_AddStringToObject(sel_props, "title", "Exercise");
    cJSON_AddItemToObject(sel, "props", sel_props);
    ASSERT(jig_selector_matches(el, sel) == 1, "props string: partial match");
    cJSON_Delete(sel);

    sel = cJSON_CreateObject();
    sel_props = cJSON_CreateObject();
    cJSON_AddStringToObject(sel_props, "title", "Meditation");
    cJSON_AddItemToObject(sel, "props", sel_props);
    ASSERT(jig_selector_matches(el, sel) == 0, "props string: mismatch");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_match_props_number(void) {
    cJSON *props = cJSON_CreateObject();
    cJSON_AddNumberToObject(props, "streak", 5);
    cJSON *el = make_element_with_props(1, "HabitCard", props, -1);

    cJSON *sel = cJSON_CreateObject();
    cJSON *sel_props = cJSON_CreateObject();
    cJSON_AddNumberToObject(sel_props, "streak", 5);
    cJSON_AddItemToObject(sel, "props", sel_props);
    ASSERT(jig_selector_matches(el, sel) == 1, "props number: match");
    cJSON_Delete(sel);

    sel = cJSON_CreateObject();
    sel_props = cJSON_CreateObject();
    cJSON_AddNumberToObject(sel_props, "streak", 10);
    cJSON_AddItemToObject(sel, "props", sel_props);
    ASSERT(jig_selector_matches(el, sel) == 0, "props number: mismatch");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_match_props_boolean(void) {
    cJSON *props = cJSON_CreateObject();
    cJSON_AddBoolToObject(props, "checked", 1);
    cJSON *el = make_element_with_props(1, "Checkbox", props, -1);

    cJSON *sel = cJSON_CreateObject();
    cJSON *sel_props = cJSON_CreateObject();
    cJSON_AddBoolToObject(sel_props, "checked", 1);
    cJSON_AddItemToObject(sel, "props", sel_props);
    ASSERT(jig_selector_matches(el, sel) == 1, "props bool: match true");
    cJSON_Delete(sel);

    sel = cJSON_CreateObject();
    sel_props = cJSON_CreateObject();
    cJSON_AddBoolToObject(sel_props, "checked", 0);
    cJSON_AddItemToObject(sel, "props", sel_props);
    ASSERT(jig_selector_matches(el, sel) == 0, "props bool: mismatch");
    cJSON_Delete(sel);
    cJSON_Delete(el);

    /* false matches false */
    cJSON *props2 = cJSON_CreateObject();
    cJSON_AddBoolToObject(props2, "checked", 0);
    cJSON *el2 = make_element_with_props(2, "Checkbox", props2, -1);
    sel = cJSON_CreateObject();
    sel_props = cJSON_CreateObject();
    cJSON_AddBoolToObject(sel_props, "checked", 0);
    cJSON_AddItemToObject(sel, "props", sel_props);
    ASSERT(jig_selector_matches(el2, sel) == 1, "props bool: match false");
    cJSON_Delete(sel);
    cJSON_Delete(el2);
}

static void test_match_props_null(void) {
    cJSON *props = cJSON_CreateObject();
    cJSON_AddNullToObject(props, "error");
    cJSON *el = make_element_with_props(1, "Status", props, -1);

    cJSON *sel = cJSON_CreateObject();
    cJSON *sel_props = cJSON_CreateObject();
    cJSON_AddNullToObject(sel_props, "error");
    cJSON_AddItemToObject(sel, "props", sel_props);
    ASSERT(jig_selector_matches(el, sel) == 1, "props null: match");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_match_props_missing_key(void) {
    cJSON *props = cJSON_CreateObject();
    cJSON_AddStringToObject(props, "title", "Exercise");
    cJSON *el = make_element_with_props(1, "HabitCard", props, -1);

    cJSON *sel = cJSON_CreateObject();
    cJSON *sel_props = cJSON_CreateObject();
    cJSON_AddStringToObject(sel_props, "nonexistent", "value");
    cJSON_AddItemToObject(sel, "props", sel_props);
    ASSERT(jig_selector_matches(el, sel) == 0, "props missing key: no match");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_match_props_type_mismatch(void) {
    cJSON *props = cJSON_CreateObject();
    cJSON_AddStringToObject(props, "count", "5");
    cJSON *el = make_element_with_props(1, "Counter", props, -1);

    cJSON *sel = cJSON_CreateObject();
    cJSON *sel_props = cJSON_CreateObject();
    cJSON_AddNumberToObject(sel_props, "count", 5);
    cJSON_AddItemToObject(sel, "props", sel_props);
    ASSERT(jig_selector_matches(el, sel) == 0, "props type mismatch: string vs number");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_match_props_no_props_on_element(void) {
    cJSON *el = make_element(1, NULL, NULL, NULL, "HabitCard", -1);
    cJSON *sel = cJSON_CreateObject();
    cJSON *sel_props = cJSON_CreateObject();
    cJSON_AddStringToObject(sel_props, "title", "Exercise");
    cJSON_AddItemToObject(sel, "props", sel_props);
    ASSERT(jig_selector_matches(el, sel) == 0, "props: no props on element");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_match_no_props_selector(void) {
    cJSON *props = cJSON_CreateObject();
    cJSON_AddStringToObject(props, "title", "Exercise");
    cJSON *el = make_element_with_props(1, "HabitCard", props, -1);

    cJSON *sel = make_selector_str("component", "HabitCard");
    ASSERT(jig_selector_matches(el, sel) == 1, "no props selector: still matches");
    cJSON_Delete(sel);
    cJSON_Delete(el);
}

static void test_find_with_props(void) {
    cJSON *elements = cJSON_CreateArray();

    cJSON *props1 = cJSON_CreateObject();
    cJSON_AddStringToObject(props1, "title", "Exercise");
    cJSON_AddItemToArray(elements, make_element_with_props(10, "HabitCard", props1, -1));

    cJSON *props2 = cJSON_CreateObject();
    cJSON_AddStringToObject(props2, "title", "Meditation");
    cJSON_AddItemToArray(elements, make_element_with_props(20, "HabitCard", props2, -1));

    cJSON *props3 = cJSON_CreateObject();
    cJSON_AddStringToObject(props3, "title", "Exercise");
    cJSON_AddItemToArray(elements, make_element_with_props(30, "HabitCard", props3, -1));

    cJSON *sel = cJSON_CreateObject();
    cJSON_AddStringToObject(sel, "component", "HabitCard");
    cJSON *sel_props = cJSON_CreateObject();
    cJSON_AddStringToObject(sel_props, "title", "Exercise");
    cJSON_AddItemToObject(sel, "props", sel_props);

    cJSON *one = jig_selector_find_one(elements, sel);
    ASSERT(one != NULL, "find with props: found element");
    cJSON *tag = cJSON_GetObjectItem(one, "reactTag");
    ASSERT(tag && tag->valueint == 10, "find with props: first match");
    cJSON_Delete(one);

    cJSON *all = jig_selector_find_all(elements, sel);
    ASSERT(cJSON_GetArraySize(all) == 2, "find_all with props: 2 matches");
    cJSON_Delete(all);

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

    /* Task 4: within scoping */
    test_within_basic();
    test_within_with_index();
    test_within_no_container();
    test_within_deep_nesting();

    /* Task: props matching */
    test_match_props_string();
    test_match_props_number();
    test_match_props_boolean();
    test_match_props_null();
    test_match_props_missing_key();
    test_match_props_type_mismatch();
    test_match_props_no_props_on_element();
    test_match_no_props_selector();
    test_find_with_props();

    if (failures == 0) printf("test_selector: all tests passed\n");
    else printf("test_selector: %d failure(s)\n", failures);
    return failures;
}
