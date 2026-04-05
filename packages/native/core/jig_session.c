#include "jig_session.h"
#include <stdlib.h>
#include <string.h>

jig_session *jig_session_create(int connection_id,
                                 jig_app_info *app_info,
                                 jig_send_fn send_text,
                                 void *send_user_data) {
    jig_session *s = calloc(1, sizeof(jig_session));
    if (!s) return NULL;

    s->connection_id = connection_id;
    s->app_info = app_info;
    s->send_text = send_text;
    s->send_user_data = send_user_data;
    s->session_id = NULL;
    s->negotiated_version = 0;
    s->enabled_domains = NULL;
    s->enabled_domain_count = 0;

    return s;
}

void jig_session_destroy(jig_session *session) {
    if (!session) return;

    free(session->session_id);

    for (int i = 0; i < session->enabled_domain_count; i++) {
        free(session->enabled_domains[i]);
    }
    free(session->enabled_domains);

    free(session);
}

int jig_session_add_domain(jig_session *session, const char *domain) {
    if (!session || !domain) return -1;

    /* check if already present */
    if (jig_session_has_domain(session, domain)) {
        return 0;
    }

    char **new_list = realloc(session->enabled_domains,
                              (size_t)(session->enabled_domain_count + 1) * sizeof(char *));
    if (!new_list) return -1;

    session->enabled_domains = new_list;
    char *dup = strdup(domain);
    if (!dup) return -1;
    session->enabled_domains[session->enabled_domain_count] = dup;
    session->enabled_domain_count++;

    return 0;
}

int jig_session_has_domain(const jig_session *session, const char *domain) {
    if (!session || !domain) return 0;

    for (int i = 0; i < session->enabled_domain_count; i++) {
        if (strcmp(session->enabled_domains[i], domain) == 0) {
            return 1;
        }
    }
    return 0;
}

int jig_session_remove_domain(jig_session *session, const char *domain) {
    if (!session || !domain) return -1;

    for (int i = 0; i < session->enabled_domain_count; i++) {
        if (strcmp(session->enabled_domains[i], domain) == 0) {
            free(session->enabled_domains[i]);

            /* shift remaining elements */
            for (int j = i; j < session->enabled_domain_count - 1; j++) {
                session->enabled_domains[j] = session->enabled_domains[j + 1];
            }
            session->enabled_domain_count--;
            return 0;
        }
    }
    return -1; /* not found */
}
