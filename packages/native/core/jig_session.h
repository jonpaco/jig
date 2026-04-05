#ifndef JIG_SESSION_H
#define JIG_SESSION_H

#include "jig_app_info.h"

typedef void (*jig_send_fn)(const char *text, void *user_data);

typedef struct jig_session {
    int connection_id;
    jig_app_info *app_info;       /* borrowed, not owned */
    jig_send_fn send_text;
    void *send_user_data;
    char *session_id;             /* NULL before handshake */
    int negotiated_version;       /* 0 before handshake */
    char **enabled_domains;
    int enabled_domain_count;
} jig_session;

jig_session *jig_session_create(int connection_id,
                                 jig_app_info *app_info,
                                 jig_send_fn send_text,
                                 void *send_user_data);

void jig_session_destroy(jig_session *session);

int jig_session_add_domain(jig_session *session, const char *domain);
int jig_session_has_domain(const jig_session *session, const char *domain);
int jig_session_remove_domain(jig_session *session, const char *domain);

#endif /* JIG_SESSION_H */
