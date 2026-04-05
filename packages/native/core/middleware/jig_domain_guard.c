#include "jig_domain_guard.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char **supported_domains;
    int domain_count;
} domain_guard_ctx;

static int domain_guard_validate(void *ctx, const char *method, cJSON *params,
                                  jig_session *session, jig_error **err) {
    (void)params;
    (void)session;

    domain_guard_ctx *dg = (domain_guard_ctx *)ctx;

    /* Only check methods ending in .enable or .disable */
    size_t len = strlen(method);
    int is_enable = (len >= 7 && strcmp(method + len - 7, ".enable") == 0);
    int is_disable = (len >= 8 && strcmp(method + len - 8, ".disable") == 0);

    if (!is_enable && !is_disable) {
        return 0;
    }

    /* Extract domain: part before first '.' */
    const char *dot = strchr(method, '.');
    if (!dot) {
        return 0;
    }

    size_t domain_len = (size_t)(dot - method);
    char domain[128];
    if (domain_len >= sizeof(domain)) domain_len = sizeof(domain) - 1;
    memcpy(domain, method, domain_len);
    domain[domain_len] = '\0';

    /* Check if domain is supported */
    for (int i = 0; i < dg->domain_count; i++) {
        if (strcmp(dg->supported_domains[i], domain) == 0) {
            return 0;
        }
    }

    *err = jig_error_unsupported_domain(domain);
    return 1;
}

jig_middleware jig_domain_guard_create(const char **supported_domains,
                                       int domain_count) {
    jig_middleware mw;
    domain_guard_ctx *ctx = calloc(1, sizeof(domain_guard_ctx));
    if (ctx) {
        ctx->supported_domains = supported_domains;
        ctx->domain_count = domain_count;
    }
    mw.validate = domain_guard_validate;
    mw.ctx = ctx;
    return mw;
}

void jig_domain_guard_destroy(jig_middleware *mw) {
    if (mw && mw->ctx) {
        free(mw->ctx);
        mw->ctx = NULL;
    }
}
