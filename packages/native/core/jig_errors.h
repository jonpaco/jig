#ifndef JIG_ERRORS_H
#define JIG_ERRORS_H

#include "vendor/cJSON/cJSON.h"

/* JSON-RPC 2.0 standard error codes */
#define JIG_ERROR_PARSE_ERROR          (-32700)
#define JIG_ERROR_INVALID_REQUEST      (-32600)
#define JIG_ERROR_METHOD_NOT_FOUND     (-32601)
#define JIG_ERROR_INVALID_PARAMS       (-32602)
#define JIG_ERROR_INTERNAL_ERROR       (-32603)

/* Jig-specific error codes (server range: -32000 to -32099) */
#define JIG_ERROR_HANDSHAKE_REQUIRED   (-32000)
#define JIG_ERROR_PROTOCOL_VERSION     (-32001)
#define JIG_ERROR_ELEMENT_NOT_FOUND    (-32002)
#define JIG_ERROR_TIMEOUT              (-32003)
#define JIG_ERROR_INVALID_SELECTOR     (-32004)
#define JIG_ERROR_UNSUPPORTED_DOMAIN   (-32005)
#define JIG_ERROR_DOMAIN_NOT_ENABLED   (-32006)

typedef struct {
    int code;
    char *message;   /* heap-allocated, owned by this struct */
    cJSON *data;     /* optional, owned by this struct; NULL if absent */
} jig_error;

/* Factory functions — caller owns the returned jig_error (free with jig_error_free). */
jig_error *jig_error_parse(const char *message);
jig_error *jig_error_invalid_request(const char *message);
jig_error *jig_error_method_not_found(const char *method);
jig_error *jig_error_invalid_params(const char *message);
jig_error *jig_error_internal(const char *message);
jig_error *jig_error_handshake_required(void);
jig_error *jig_error_protocol_version(int requested, int supported_min, int supported_max);
jig_error *jig_error_element_not_found(const char *message, cJSON *data);
jig_error *jig_error_timeout(const char *message);
jig_error *jig_error_invalid_selector(const char *message);
jig_error *jig_error_unsupported_domain(const char *domain);
jig_error *jig_error_domain_not_enabled(const char *domain);

/* Convert error to a JSON-RPC 2.0 error object: {"code":..,"message":..,"data":..} */
cJSON *jig_error_to_json(const jig_error *err);

/* Free a jig_error and its owned resources. */
void jig_error_free(jig_error *err);

#endif /* JIG_ERRORS_H */
