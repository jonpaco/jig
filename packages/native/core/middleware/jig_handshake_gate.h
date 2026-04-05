#ifndef JIG_HANDSHAKE_GATE_H
#define JIG_HANDSHAKE_GATE_H

#include "../jig_middleware.h"

/*
 * Create a handshake gate middleware.
 * Allows client.hello always; rejects all other methods before handshake.
 * The returned middleware's ctx is NULL (no state needed).
 */
jig_middleware jig_handshake_gate_create(void);

#endif /* JIG_HANDSHAKE_GATE_H */
