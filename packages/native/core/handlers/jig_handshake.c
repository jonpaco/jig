#include "jig_handshake.h"
#include "../jig_protocol.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static cJSON *handshake_handle(jig_handler *self, cJSON *params,
                                jig_session *session, jig_error **err) {
    (void)self;
    (void)session;

    /* Validate params */
    if (!params || !cJSON_IsObject(params)) {
        *err = jig_error_invalid_params("Invalid client.hello params");
        return NULL;
    }

    cJSON *proto = cJSON_GetObjectItem(params, "protocol");
    if (!proto || !cJSON_IsObject(proto)) {
        *err = jig_error_invalid_params("Invalid client.hello params");
        return NULL;
    }

    cJSON *version = cJSON_GetObjectItem(proto, "version");
    if (!version || !cJSON_IsNumber(version) || version->valueint == 0) {
        *err = jig_error_invalid_params("Invalid client.hello params");
        return NULL;
    }

    int requested = version->valueint;

    /* Check version compatibility */
    if (requested < JIG_PROTOCOL_VERSION || requested > JIG_PROTOCOL_VERSION) {
        *err = jig_error_protocol_version(requested,
                                           JIG_PROTOCOL_VERSION,
                                           JIG_PROTOCOL_VERSION);
        return NULL;
    }

    /* Generate session ID: sess_ + 8 hex chars.
       The LCG below is intentionally non-cryptographic — session IDs are
       used for correlation/logging, not for security or authentication. */
    unsigned char bytes[4];
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)(size_t)session;
    for (int i = 0; i < 4; i++) {
        seed = seed * 1103515245 + 12345;
        bytes[i] = (unsigned char)((seed >> 16) & 0xFF);
    }

    char session_id[14]; /* "sess_" + 8 hex + NUL */
    snprintf(session_id, sizeof(session_id), "sess_%02x%02x%02x%02x",
             bytes[0], bytes[1], bytes[2], bytes[3]);

    /* Build result */
    cJSON *result = cJSON_CreateObject();
    cJSON_AddStringToObject(result, "sessionId", session_id);

    cJSON *negotiated = cJSON_CreateObject();
    cJSON_AddNumberToObject(negotiated, "protocol", requested);
    cJSON_AddItemToObject(result, "negotiated", negotiated);

    cJSON_AddItemToObject(result, "enabled", cJSON_CreateArray());
    cJSON_AddItemToObject(result, "rejected", cJSON_CreateArray());

    return result;
}

jig_handler *jig_handshake_create(void) {
    jig_handler *h = calloc(1, sizeof(jig_handler));
    if (!h) return NULL;

    h->method = "client.hello";
    h->thread_target = JIG_THREAD_WEBSOCKET;
    h->handle = handshake_handle;
    h->user_data = NULL;

    return h;
}

void jig_handshake_destroy(jig_handler *handler) {
    free(handler);
}
