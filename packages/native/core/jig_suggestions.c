#include "jig_suggestions.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>

static int levenshtein(const char *s, const char *t) {
    int m = (int)strlen(s);
    int n = (int)strlen(t);
    int *row = calloc((size_t)(n + 1), sizeof(int));
    if (!row) return INT_MAX;
    for (int j = 0; j <= n; j++) row[j] = j;
    for (int i = 1; i <= m; i++) {
        int prev = row[0];
        row[0] = i;
        for (int j = 1; j <= n; j++) {
            int cost = (s[i - 1] == t[j - 1]) ? 0 : 1;
            int temp = row[j];
            int del = row[j] + 1;
            int ins = row[j - 1] + 1;
            int sub = prev + cost;
            int min = del < ins ? del : ins;
            row[j] = min < sub ? min : sub;
            prev = temp;
        }
    }
    int result = row[n];
    free(row);
    return result;
}

typedef struct { int element_index; int distance; } scored_element;

static int compare_scored(const void *a, const void *b) {
    return ((const scored_element *)a)->distance - ((const scored_element *)b)->distance;
}

static const char *get_search_string(const cJSON *selector) {
    static const char *fields[] = {"testID", "text"};
    for (int i = 0; i < 2; i++) {
        cJSON *val = cJSON_GetObjectItemCaseSensitive(selector, fields[i]);
        if (val && cJSON_IsString(val)) return val->valuestring;
    }
    return NULL;
}

cJSON *jig_suggest_elements(const cJSON *elements, const cJSON *selector,
                            int max_suggestions) {
    cJSON *result = cJSON_CreateArray();
    const char *search = get_search_string(selector);
    if (!search) return result;
    int count = cJSON_GetArraySize(elements);
    if (count == 0) return result;
    scored_element *scores = calloc((size_t)count, sizeof(scored_element));
    if (!scores) return result;
    int num_scored = 0;
    int idx = 0;
    cJSON *el = NULL;
    cJSON_ArrayForEach(el, elements) {
        cJSON *vis = cJSON_GetObjectItemCaseSensitive(el, "visible");
        if (!vis || !cJSON_IsTrue(vis)) { idx++; continue; }
        int best = INT_MAX;
        static const char *compare_fields[] = {"testID", "text"};
        for (int f = 0; f < 2; f++) {
            cJSON *field = cJSON_GetObjectItemCaseSensitive(el, compare_fields[f]);
            if (field && cJSON_IsString(field)) {
                int d = levenshtein(search, field->valuestring);
                if (d < best) best = d;
            }
        }
        if (best < INT_MAX) {
            scores[num_scored].element_index = idx;
            scores[num_scored].distance = best;
            num_scored++;
        }
        idx++;
    }
    if (num_scored > 0) {
        qsort(scores, (size_t)num_scored, sizeof(scored_element), compare_scored);
    }
    int limit = num_scored < max_suggestions ? num_scored : max_suggestions;
    for (int i = 0; i < limit; i++) {
        cJSON *source = cJSON_GetArrayItem(elements, scores[i].element_index);
        cJSON *suggestion = cJSON_CreateObject();
        cJSON *tid = cJSON_GetObjectItemCaseSensitive(source, "testID");
        if (tid && cJSON_IsString(tid)) cJSON_AddStringToObject(suggestion, "testID", tid->valuestring);
        cJSON_AddItemToArray(result, suggestion);
    }
    free(scores);
    return result;
}
