#include "test_util.h"
#include "../jig_suggestions.h"
#include <string.h>

static cJSON *make_visible_element(const char *testID, const char *component) {
    cJSON *el = cJSON_CreateObject();
    cJSON_AddNumberToObject(el, "reactTag", 1);
    if (testID) cJSON_AddStringToObject(el, "testID", testID);
    if (component) cJSON_AddStringToObject(el, "component", component);
    cJSON_AddBoolToObject(el, "visible", 1);
    return el;
}

static void test_suggest_similar_testID(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_visible_element("submit-button", NULL));
    cJSON_AddItemToArray(elements, make_visible_element("cancel-button", NULL));
    cJSON_AddItemToArray(elements, make_visible_element("submit-habit-button", NULL));
    cJSON_AddItemToArray(elements, make_visible_element("header-text", NULL));
    cJSON *selector = cJSON_CreateObject();
    cJSON_AddStringToObject(selector, "testID", "submit-btn");
    cJSON *suggestions = jig_suggest_elements(elements, selector, 3);
    ASSERT(cJSON_IsArray(suggestions), "suggestions: returns array");
    ASSERT(cJSON_GetArraySize(suggestions) > 0, "suggestions: non-empty");
    ASSERT(cJSON_GetArraySize(suggestions) <= 3, "suggestions: respects max");
    cJSON *first = cJSON_GetArrayItem(suggestions, 0);
    ASSERT(first != NULL, "suggestions: has first item");
    cJSON_Delete(suggestions);
    cJSON_Delete(selector);
    cJSON_Delete(elements);
}

static void test_suggest_by_component(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_visible_element(NULL, "HabitCard"));
    cJSON_AddItemToArray(elements, make_visible_element(NULL, "HabitList"));
    cJSON_AddItemToArray(elements, make_visible_element(NULL, "SettingsScreen"));
    cJSON *selector = cJSON_CreateObject();
    cJSON_AddStringToObject(selector, "component", "HabitCrd");
    cJSON *suggestions = jig_suggest_elements(elements, selector, 5);
    ASSERT(cJSON_GetArraySize(suggestions) > 0, "component suggestions: non-empty");
    cJSON *first = cJSON_GetArrayItem(suggestions, 0);
    cJSON *comp = cJSON_GetObjectItem(first, "component");
    ASSERT(comp && strcmp(comp->valuestring, "HabitCard") == 0,
           "component suggestions: HabitCard is closest");
    cJSON_Delete(suggestions);
    cJSON_Delete(selector);
    cJSON_Delete(elements);
}

static void test_suggest_skips_invisible(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON *visible = make_visible_element("submit-button", NULL);
    cJSON_AddItemToArray(elements, visible);
    cJSON *invisible = cJSON_CreateObject();
    cJSON_AddNumberToObject(invisible, "reactTag", 2);
    cJSON_AddStringToObject(invisible, "testID", "submit-btn");
    cJSON_AddBoolToObject(invisible, "visible", 0);
    cJSON_AddItemToArray(elements, invisible);
    cJSON *selector = cJSON_CreateObject();
    cJSON_AddStringToObject(selector, "testID", "submit-btn");
    cJSON *suggestions = jig_suggest_elements(elements, selector, 5);
    int found_invisible = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, suggestions) {
        cJSON *tid = cJSON_GetObjectItem(item, "testID");
        if (tid && strcmp(tid->valuestring, "submit-btn") == 0) found_invisible = 1;
    }
    ASSERT(found_invisible == 0, "suggestions: skips invisible elements");
    cJSON_Delete(suggestions);
    cJSON_Delete(selector);
    cJSON_Delete(elements);
}

static void test_suggest_empty_elements(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON *selector = cJSON_CreateObject();
    cJSON_AddStringToObject(selector, "testID", "anything");
    cJSON *suggestions = jig_suggest_elements(elements, selector, 5);
    ASSERT(cJSON_GetArraySize(suggestions) == 0, "empty elements: returns empty array");
    cJSON_Delete(suggestions);
    cJSON_Delete(selector);
    cJSON_Delete(elements);
}

static void test_suggest_no_selector_value(void) {
    cJSON *elements = cJSON_CreateArray();
    cJSON_AddItemToArray(elements, make_visible_element("card", NULL));
    cJSON *selector = cJSON_CreateObject();
    cJSON *suggestions = jig_suggest_elements(elements, selector, 5);
    ASSERT(cJSON_GetArraySize(suggestions) == 0,
           "no selector value: returns empty (nothing to fuzzy match)");
    cJSON_Delete(suggestions);
    cJSON_Delete(selector);
    cJSON_Delete(elements);
}

int main(void) {
    test_suggest_similar_testID();
    test_suggest_by_component();
    test_suggest_skips_invisible();
    test_suggest_empty_elements();
    test_suggest_no_selector_value();
    if (failures == 0) printf("test_suggestions: all tests passed\n");
    else printf("test_suggestions: %d failure(s)\n", failures);
    return failures;
}
