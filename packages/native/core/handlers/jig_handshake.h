#ifndef JIG_HANDSHAKE_H
#define JIG_HANDSHAKE_H

#include "../jig_handler.h"

/*
 * Create the client.hello handshake handler.
 * Caller owns the returned handler (free with jig_handshake_destroy).
 */
jig_handler *jig_handshake_create(void);

void jig_handshake_destroy(jig_handler *handler);

#endif /* JIG_HANDSHAKE_H */
