#include "jig_handshake_gate.h"
#include <string.h>

static int handshake_gate_validate(void *ctx, const char *method, cJSON *params,
                                    jig_session *session, jig_error **err) {
    (void)ctx;
    (void)params;

    if (strcmp(method, "client.hello") == 0) {
        return 0;
    }

    if (session->session_id == NULL) {
        *err = jig_error_handshake_required();
        return 1;
    }

    return 0;
}

jig_middleware jig_handshake_gate_create(void) {
    jig_middleware mw;
    mw.validate = handshake_gate_validate;
    mw.ctx = NULL;
    return mw;
}
