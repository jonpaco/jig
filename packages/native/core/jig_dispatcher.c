#include "jig_dispatcher.h"
#include "jig_platform.h"
#include "jig_protocol.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* --- Internal helpers --- */

static void send_json(cJSON *json, jig_session *session) {
    char *text = cJSON_PrintUnformatted(json);
    if (text) {
        session->send_text(text, session->send_user_data);
        free(text);
    }
}

static void send_result(cJSON *result, cJSON *id, jig_session *session) {
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "jsonrpc", "2.0");
    cJSON_AddItemToObject(resp, "id", cJSON_Duplicate(id, 1));
    if (result) {
        cJSON_AddItemToObject(resp, "result", cJSON_Duplicate(result, 1));
    } else {
        cJSON_AddNullToObject(resp, "result");
    }
    send_json(resp, session);
    cJSON_Delete(resp);
}

static void send_error(jig_error *error, cJSON *id, jig_session *session) {
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "jsonrpc", "2.0");

    if (id) {
        cJSON_AddItemToObject(resp, "id", cJSON_Duplicate(id, 1));
    } else {
        cJSON_AddNullToObject(resp, "id");
    }

    cJSON *err_json = jig_error_to_json(error);
    cJSON_AddItemToObject(resp, "error", err_json);

    send_json(resp, session);
    cJSON_Delete(resp);
}

/* --- Public API --- */

jig_dispatcher_config *jig_dispatcher_create(
    jig_middleware *middlewares, int middleware_count,
    jig_handler **handlers, int handler_count,
    const char **supported_domains, int domain_count) {

    jig_dispatcher_config *cfg = calloc(1, sizeof(jig_dispatcher_config));
    if (!cfg) return NULL;

    cfg->middlewares = middlewares;
    cfg->middleware_count = middleware_count;
    cfg->handlers = handlers;
    cfg->handler_count = handler_count;
    cfg->supported_domains = supported_domains;
    cfg->domain_count = domain_count;

    /* Build command_names: all handler methods except "client.hello" and
     * internal "jig.internal.*" handlers (not exposed to external clients) */
    int cmd_count = 0;
    for (int i = 0; i < handler_count; i++) {
        if (strcmp(handlers[i]->method, "client.hello") != 0 &&
            strncmp(handlers[i]->method, "jig.internal.", 13) != 0) {
            cmd_count++;
        }
    }

    if (cmd_count > 0) {
        cfg->command_names = calloc((size_t)cmd_count, sizeof(const char *));
        int idx = 0;
        for (int i = 0; i < handler_count; i++) {
            if (strcmp(handlers[i]->method, "client.hello") != 0 &&
                strncmp(handlers[i]->method, "jig.internal.", 13) != 0) {
                cfg->command_names[idx++] = handlers[i]->method;
            }
        }
    }
    cfg->command_count = cmd_count;

    return cfg;
}

void jig_dispatcher_destroy(jig_dispatcher_config *config) {
    if (!config) return;
    free(config->command_names);
    free(config);
}

void jig_dispatcher_handle_open(jig_dispatcher_config *config,
                                 jig_session *session) {
    cJSON *msg = cJSON_CreateObject();
    cJSON_AddStringToObject(msg, "jsonrpc", "2.0");
    cJSON_AddStringToObject(msg, "method", "server.hello");

    cJSON *params = cJSON_CreateObject();

    /* protocol info */
    cJSON *proto = cJSON_CreateObject();
    cJSON_AddStringToObject(proto, "name", "jig");
    cJSON_AddNumberToObject(proto, "version", JIG_PROTOCOL_VERSION);
    cJSON_AddNumberToObject(proto, "min", JIG_PROTOCOL_VERSION);
    cJSON_AddNumberToObject(proto, "max", JIG_PROTOCOL_VERSION);
    cJSON_AddItemToObject(params, "protocol", proto);

    /* server version */
    cJSON_AddStringToObject(params, "server", JIG_SERVER_VERSION);

    /* app info */
    cJSON *app_json = jig_app_info_to_json(session->app_info);
    cJSON_AddItemToObject(params, "app", app_json);

    /* commands */
    cJSON *commands = cJSON_CreateArray();
    for (int i = 0; i < config->command_count; i++) {
        cJSON_AddItemToArray(commands, cJSON_CreateString(config->command_names[i]));
    }
    cJSON_AddItemToObject(params, "commands", commands);

    /* domains */
    cJSON *domains = cJSON_CreateArray();
    for (int i = 0; i < config->domain_count; i++) {
        cJSON_AddItemToArray(domains, cJSON_CreateString(config->supported_domains[i]));
    }
    cJSON_AddItemToObject(params, "domains", domains);

    cJSON_AddItemToObject(msg, "params", params);

    send_json(msg, session);
    cJSON_Delete(msg);
}

void jig_dispatcher_run_on_main(void *raw_ctx) {
    jig_async_dispatch_ctx *ctx = (jig_async_dispatch_ctx *)raw_ctx;

    jig_error *handler_err = NULL;
    cJSON *result = ctx->handler->handle(ctx->handler, ctx->params,
                                          ctx->session, &handler_err);

    if (handler_err) {
        if (ctx->id) {
            send_error(handler_err, ctx->id, ctx->session);
        }
        jig_error_free(handler_err);
    } else if (ctx->id) {
        send_result(result, ctx->id, ctx->session);
    }

    if (result) cJSON_Delete(result);
    if (ctx->params) cJSON_Delete(ctx->params);
    if (ctx->id) cJSON_Delete(ctx->id);
    free(ctx);
}

/*
 * Dispatch a JSON-RPC 2.0 message. Executes synchronously on the caller's
 * thread, except for JIG_THREAD_MAIN handlers when run_on_main_thread is set.
 */
void jig_dispatcher_dispatch(jig_dispatcher_config *config,
                              const char *text,
                              jig_session *session) {
    /* 1. Parse JSON */
    cJSON *json = cJSON_Parse(text);
    if (!json) {
        jig_error *err = jig_error_parse("Malformed JSON");
        send_error(err, NULL, session);
        jig_error_free(err);
        return;
    }

    /* 2. Validate jsonrpc and method */
    cJSON *jsonrpc = cJSON_GetObjectItem(json, "jsonrpc");
    cJSON *method_item = cJSON_GetObjectItem(json, "method");

    if (!jsonrpc || !cJSON_IsString(jsonrpc) ||
        strcmp(jsonrpc->valuestring, "2.0") != 0 ||
        !method_item || !cJSON_IsString(method_item)) {

        cJSON *id = cJSON_GetObjectItem(json, "id");
        jig_error *err = jig_error_invalid_request(
            "Missing or invalid 'jsonrpc' or 'method' field");
        send_error(err, id, session);
        jig_error_free(err);
        cJSON_Delete(json);
        return;
    }

    const char *method = method_item->valuestring;
    cJSON *id = cJSON_GetObjectItem(json, "id");
    cJSON *params = cJSON_GetObjectItem(json, "params");
    int is_command = (id != NULL);

    /* 3. Run middleware chain */
    for (int i = 0; i < config->middleware_count; i++) {
        jig_error *mw_err = NULL;
        int rc = config->middlewares[i].validate(
            config->middlewares[i].ctx, method, params, session, &mw_err);
        if (rc != 0 && mw_err) {
            if (is_command) {
                send_error(mw_err, id, session);
            }
            jig_error_free(mw_err);
            cJSON_Delete(json);
            return;
        }
    }

    /* 4. Look up handler */
    jig_handler *handler = NULL;
    for (int i = 0; i < config->handler_count; i++) {
        if (strcmp(config->handlers[i]->method, method) == 0) {
            handler = config->handlers[i];
            break;
        }
    }

    if (!handler) {
        if (is_command) {
            jig_error *err = jig_error_method_not_found(method);
            send_error(err, id, session);
            jig_error_free(err);
        }
        cJSON_Delete(json);
        return;
    }

    /* 5. Execute handler — async for JIG_THREAD_MAIN, sync otherwise */
    const jig_platform_ops *plat_ops = jig_platform_get_ops();
    if (handler->thread_target == JIG_THREAD_MAIN &&
        plat_ops && plat_ops->run_on_main_thread) {

        jig_async_dispatch_ctx *ctx = calloc(1, sizeof(jig_async_dispatch_ctx));
        ctx->handler = handler;
        ctx->session = session;

        /* Transfer ownership of params: detach from parsed json */
        if (params) {
            ctx->params = cJSON_DetachItemFromObject(json, "params");
        }
        /* Clone the id for response building */
        if (id) {
            ctx->id = cJSON_Duplicate(id, 1);
        }

        plat_ops->run_on_main_thread(jig_dispatcher_run_on_main, ctx);

        /* Clean up the parsed message (params already detached) */
        cJSON_Delete(json);
        return;
    }

    jig_error *handler_err = NULL;
    cJSON *result = handler->handle(handler, params, session, &handler_err);

    if (handler_err) {
        if (is_command) {
            send_error(handler_err, id, session);
        }
        jig_error_free(handler_err);
        cJSON_Delete(json);
        return;
    }

    /* 6. Special case: update session after successful client.hello */
    if (strcmp(method, "client.hello") == 0 && result) {
        cJSON *sid = cJSON_GetObjectItem(result, "sessionId");
        if (sid && cJSON_IsString(sid)) {
            free(session->session_id);
            session->session_id = strdup(sid->valuestring);
        }
        cJSON *neg = cJSON_GetObjectItem(result, "negotiated");
        if (neg) {
            cJSON *neg_proto = cJSON_GetObjectItem(neg, "protocol");
            if (neg_proto && cJSON_IsNumber(neg_proto)) {
                session->negotiated_version = neg_proto->valueint;
            }
        }
    }

    /* 7. Send result (only for commands) */
    if (is_command) {
        send_result(result, id, session);
    }

    if (result) cJSON_Delete(result);
    cJSON_Delete(json);
}
