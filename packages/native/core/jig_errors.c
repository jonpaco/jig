#include "jig_errors.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static jig_error *make_error(int code, const char *message, cJSON *data) {
    jig_error *err = calloc(1, sizeof(jig_error));
    if (!err) return NULL;
    err->code = code;
    err->message = strdup(message);
    err->data = data;
    return err;
}

jig_error *jig_error_parse(const char *message) {
    return make_error(JIG_ERROR_PARSE_ERROR, message, NULL);
}

jig_error *jig_error_invalid_request(const char *message) {
    return make_error(JIG_ERROR_INVALID_REQUEST, message, NULL);
}

jig_error *jig_error_method_not_found(const char *method) {
    char buf[256];
    snprintf(buf, sizeof(buf), "Method '%s' not found", method);
    return make_error(JIG_ERROR_METHOD_NOT_FOUND, buf, NULL);
}

jig_error *jig_error_invalid_params(const char *message) {
    return make_error(JIG_ERROR_INVALID_PARAMS, message, NULL);
}

jig_error *jig_error_internal(const char *message) {
    return make_error(JIG_ERROR_INTERNAL_ERROR, message, NULL);
}

jig_error *jig_error_handshake_required(void) {
    return make_error(JIG_ERROR_HANDSHAKE_REQUIRED,
                      "Handshake required before sending commands", NULL);
}

jig_error *jig_error_protocol_version(int requested, int supported_min, int supported_max) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "Protocol version %d not supported. Server supports version %d.",
             requested, supported_min);

    cJSON *data = cJSON_CreateObject();
    cJSON *supported = cJSON_CreateObject();
    cJSON_AddNumberToObject(supported, "min", supported_min);
    cJSON_AddNumberToObject(supported, "max", supported_max);
    cJSON_AddItemToObject(data, "supported", supported);

    return make_error(JIG_ERROR_PROTOCOL_VERSION, buf, data);
}

jig_error *jig_error_element_not_found(const char *message, cJSON *data) {
    return make_error(JIG_ERROR_ELEMENT_NOT_FOUND, message, data);
}

jig_error *jig_error_timeout(const char *message) {
    return make_error(JIG_ERROR_TIMEOUT, message, NULL);
}

jig_error *jig_error_invalid_selector(const char *message) {
    return make_error(JIG_ERROR_INVALID_SELECTOR, message, NULL);
}

jig_error *jig_error_unsupported_domain(const char *domain) {
    char buf[256];
    snprintf(buf, sizeof(buf), "Domain '%s' is not supported", domain);
    return make_error(JIG_ERROR_UNSUPPORTED_DOMAIN, buf, NULL);
}

jig_error *jig_error_domain_not_enabled(const char *domain) {
    char buf[256];
    snprintf(buf, sizeof(buf), "Domain '%s' must be enabled before use", domain);
    return make_error(JIG_ERROR_DOMAIN_NOT_ENABLED, buf, NULL);
}

cJSON *jig_error_to_json(const jig_error *err) {
    if (!err) return NULL;

    cJSON *obj = cJSON_CreateObject();
    if (!obj) return NULL;

    cJSON_AddNumberToObject(obj, "code", err->code);
    cJSON_AddStringToObject(obj, "message", err->message);

    if (err->data) {
        cJSON_AddItemToObject(obj, "data", cJSON_Duplicate(err->data, 1));
    }

    return obj;
}

void jig_error_free(jig_error *err) {
    if (!err) return;
    free(err->message);
    if (err->data) {
        cJSON_Delete(err->data);
    }
    free(err);
}
