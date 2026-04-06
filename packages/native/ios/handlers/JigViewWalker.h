#ifndef JIG_VIEW_WALKER_H
#define JIG_VIEW_WALKER_H

#include "../../core/vendor/cJSON/cJSON.h"

/*
 * Walk the native UIView tree and return a cJSON array of elements.
 * Each element: { reactTag, parentReactTag, testID?, text?, role?, frame, visible }
 * MUST be called on the main thread.
 * Caller owns the returned cJSON.
 */
cJSON *jig_ios_walk_views(void);

#endif
