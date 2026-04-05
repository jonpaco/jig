#ifndef JIG_SERVER_H
#define JIG_SERVER_H

#include "jig_dispatcher.h"
#include "jig_session.h"

typedef struct jig_server jig_server;

/* Create server on given port with dispatcher config */
jig_server *jig_server_create(int port, jig_dispatcher_config *dispatcher, jig_app_info *app_info);

/* Start listening (blocks in event loop -- call from dedicated thread) */
int jig_server_start(jig_server *server);

/* Signal the event loop to stop */
void jig_server_stop(jig_server *server);

/* Clean up */
void jig_server_destroy(jig_server *server);

#endif /* JIG_SERVER_H */
