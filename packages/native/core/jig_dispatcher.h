#ifndef JIG_DISPATCHER_H
#define JIG_DISPATCHER_H

#include "jig_handler.h"
#include "jig_middleware.h"
#include "jig_session.h"

typedef struct jig_dispatcher_config {
    jig_middleware *middlewares;
    int middleware_count;
    jig_handler **handlers;
    int handler_count;
    const char **supported_domains;
    int domain_count;
    const char **command_names;  /* auto-populated, excluding "client.hello" */
    int command_count;
} jig_dispatcher_config;

typedef struct {
    jig_handler *handler;
    cJSON *params;        /* ownership transferred from dispatcher */
    jig_session *session;
    cJSON *id;            /* cloned from parsed message */
} jig_async_dispatch_ctx;

void jig_dispatcher_run_on_main(void *raw_ctx);

/*
 * Create a dispatcher config. Builds the handler lookup table and populates
 * command_names from the handler methods (excluding "client.hello").
 * Caller owns the returned config (free with jig_dispatcher_destroy).
 *
 * IMPORTANT: The dispatcher stores the handlers and middlewares pointers
 * directly — it does NOT copy the arrays. The caller must ensure these
 * arrays outlive the dispatcher (use static or heap allocation, not
 * stack-local arrays in a function that returns).
 */
jig_dispatcher_config *jig_dispatcher_create(
    jig_middleware *middlewares, int middleware_count,
    jig_handler **handlers, int handler_count,
    const char **supported_domains, int domain_count);

void jig_dispatcher_destroy(jig_dispatcher_config *config);

/*
 * Called when a new WebSocket connection opens. Sends the server.hello
 * message to the client via session->send_text.
 */
void jig_dispatcher_handle_open(jig_dispatcher_config *config,
                                 jig_session *session);

/*
 * Dispatch a JSON-RPC 2.0 message received from the client.
 *
 * Threading note: The dispatcher always executes synchronously on the
 * caller's thread. The handler's thread_target field is informational only;
 * it is the caller's responsibility to invoke this function on the
 * appropriate thread/queue.
 */
void jig_dispatcher_dispatch(jig_dispatcher_config *config,
                              const char *text,
                              jig_session *session);

#endif /* JIG_DISPATCHER_H */
