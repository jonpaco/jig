// ios/JigBootstrap.h
//
// Shared server bootstrap — used by both JigBridge (React Native module)
// and JigStandaloneInit (DYLD_INSERT_LIBRARIES injection).

#ifndef JIG_BOOTSTRAP_H
#define JIG_BOOTSTRAP_H

#include "../core/jig_server.h"

// Bootstrap the Jig server with the given app info. Returns the server instance.
// Starts the server on a background dispatch queue.
// Caller is responsible for storing the returned pointer for later stop/destroy.
jig_server *jig_bootstrap_server(const char *name, const char *bundle_id,
                                  const char *rn_version, const char *expo_version,
                                  int port);

// Stop and destroy a running server.
void jig_bootstrap_stop(jig_server *server);

#endif
