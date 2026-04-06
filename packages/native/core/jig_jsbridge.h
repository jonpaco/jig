#ifndef JIG_JSBRIDGE_H
#define JIG_JSBRIDGE_H

#include "jig_handler.h"
#include "jig_session.h"
#include <stdbool.h>

typedef struct jig_jsbridge jig_jsbridge;

jig_jsbridge *jig_jsbridge_create(void);
void jig_jsbridge_destroy(jig_jsbridge *bridge);

#define JIG_JSBRIDGE_HANDLER_COUNT 2
jig_handler **jig_jsbridge_get_handlers(jig_jsbridge *bridge);

cJSON *jig_jsbridge_walk_fibers(jig_jsbridge *bridge, int timeout_ms, bool include_props);

void jig_jsbridge_on_disconnect(jig_jsbridge *bridge, jig_session *session);

#endif /* JIG_JSBRIDGE_H */
