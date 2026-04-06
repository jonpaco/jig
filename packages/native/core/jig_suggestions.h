#ifndef JIG_SUGGESTIONS_H
#define JIG_SUGGESTIONS_H

#include "vendor/cJSON/cJSON.h"

cJSON *jig_suggest_elements(const cJSON *elements, const cJSON *selector,
                            int max_suggestions);

#endif
