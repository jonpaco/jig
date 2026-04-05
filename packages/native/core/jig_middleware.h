#ifndef JIG_MIDDLEWARE_H
#define JIG_MIDDLEWARE_H

#include "vendor/cJSON/cJSON.h"
#include "jig_errors.h"
#include "jig_session.h"

/* Return 0 = pass, non-zero = reject (set *err) */
typedef int (*jig_middleware_fn)(void *ctx, const char *method, cJSON *params,
                                 jig_session *session, jig_error **err);

typedef struct jig_middleware {
    jig_middleware_fn validate;
    void *ctx;                       /* middleware-specific state */
} jig_middleware;

#endif /* JIG_MIDDLEWARE_H */
