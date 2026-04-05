#ifndef JIG_HANDLER_H
#define JIG_HANDLER_H

#include "vendor/cJSON/cJSON.h"
#include "jig_errors.h"
#include "jig_session.h"

typedef enum {
    JIG_THREAD_WEBSOCKET = 0,
    JIG_THREAD_MAIN = 1,
} jig_thread_target;

typedef struct jig_handler {
    const char *method;              /* e.g., "client.hello" */
    jig_thread_target thread_target;
    /* Returns a cJSON* result on success (caller owns), sets *err on failure */
    cJSON *(*handle)(struct jig_handler *self, cJSON *params,
                     jig_session *session, jig_error **err);
    void *user_data;                 /* handler-specific state */
} jig_handler;

#endif /* JIG_HANDLER_H */
