#ifndef JIG_SELECTOR_H
#define JIG_SELECTOR_H

#include "vendor/cJSON/cJSON.h"
#include <stdbool.h>

bool jig_selector_matches(const cJSON *element, const cJSON *selector);
cJSON *jig_selector_find_one(const cJSON *elements, const cJSON *selector);
cJSON *jig_selector_find_all(const cJSON *elements, const cJSON *selector);

#endif
