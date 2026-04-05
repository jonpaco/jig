#include "jig_server.h"
#include <libwebsockets.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

/* --- Per-connection send queue --- */

typedef struct jig_send_node {
    char *text;
    struct jig_send_node *next;
} jig_send_node;

typedef struct jig_send_queue {
    jig_send_node *head;
    jig_send_node *tail;
} jig_send_queue;

static void queue_init(jig_send_queue *q) {
    q->head = NULL;
    q->tail = NULL;
}

static void queue_push(jig_send_queue *q, const char *text) {
    jig_send_node *node = malloc(sizeof(jig_send_node));
    if (!node) return;
    node->text = strdup(text);
    if (!node->text) { free(node); return; }
    node->next = NULL;
    if (q->tail) {
        q->tail->next = node;
    } else {
        q->head = node;
    }
    q->tail = node;
}

static char *queue_pop(jig_send_queue *q) {
    if (!q->head) return NULL;
    jig_send_node *node = q->head;
    q->head = node->next;
    if (!q->head) q->tail = NULL;
    char *text = node->text;
    free(node);
    return text;
}

static void queue_free(jig_send_queue *q) {
    char *text;
    while ((text = queue_pop(q)) != NULL) {
        free(text);
    }
}

/* --- Per-connection user data (stored in lws per-session allocation) --- */

typedef struct {
    jig_session *session;
    jig_send_queue send_queue;
    jig_server *server;
    struct lws *wsi;
} jig_conn_data;

/* --- Server struct --- */

struct jig_server {
    int port;
    jig_dispatcher_config *dispatcher;
    jig_app_info *app_info;
    struct lws_context *lws_ctx;
    _Atomic int stop_flag;
    int next_connection_id;
};

/* --- Send callback wired into jig_session --- */

static void server_send_text(const char *text, void *user_data) {
    jig_conn_data *conn = (jig_conn_data *)user_data;
    queue_push(&conn->send_queue, text);
    if (conn->wsi) {
        lws_callback_on_writable(conn->wsi);
    }
}

/* --- libwebsockets protocol callback --- */

static int callback_jig(struct lws *wsi, enum lws_callback_reasons reason,
                        void *user, void *in, size_t len) {
    jig_conn_data *conn = (jig_conn_data *)user;

    /* Retrieve server pointer stored via lws_context_creation_info.user */
    struct lws_context *ctx = lws_get_context(wsi);
    jig_server *server = ctx ? (jig_server *)lws_context_user(ctx) : NULL;

    switch (reason) {
    case LWS_CALLBACK_ESTABLISHED: {
        if (!server) return -1;
        memset(conn, 0, sizeof(*conn));
        conn->server = server;
        conn->wsi = wsi;
        queue_init(&conn->send_queue);
        conn->session = jig_session_create(
            server->next_connection_id++,
            server->app_info,
            server_send_text,
            conn
        );
        if (!conn->session) return -1;
        jig_dispatcher_handle_open(server->dispatcher, conn->session);
        break;
    }

    case LWS_CALLBACK_RECEIVE: {
        if (!conn || !conn->session || !server) return -1;
        /* in points to received text, len is its length.
         * We need a null-terminated copy. */
        char *text = malloc(len + 1);
        if (!text) return -1;
        memcpy(text, in, len);
        text[len] = '\0';
        jig_dispatcher_dispatch(server->dispatcher, text, conn->session);
        free(text);
        break;
    }

    case LWS_CALLBACK_SERVER_WRITEABLE: {
        if (!conn) break;
        char *text = queue_pop(&conn->send_queue);
        if (text) {
            size_t text_len = strlen(text);
            /* lws requires LWS_PRE bytes of padding before the data */
            unsigned char *buf = malloc(LWS_PRE + text_len);
            if (buf) {
                memcpy(buf + LWS_PRE, text, text_len);
                lws_write(wsi, buf + LWS_PRE, text_len, LWS_WRITE_TEXT);
                free(buf);
            }
            free(text);
            /* If more messages queued, request another writable callback */
            if (conn->send_queue.head) {
                lws_callback_on_writable(wsi);
            }
        }
        break;
    }

    case LWS_CALLBACK_CLOSED: {
        /* TODO: notify dispatcher of disconnect when session tracking is added */
        if (conn) {
            if (conn->session) {
                jig_session_destroy(conn->session);
                conn->session = NULL;
            }
            queue_free(&conn->send_queue);
            conn->wsi = NULL;
        }
        break;
    }

    default:
        break;
    }

    return 0;
}

/* --- Protocol table (file-scope, used by jig_server_create) --- */

static const struct lws_protocols protocols[] = {
    {
        "jig-protocol",
        callback_jig,
        sizeof(jig_conn_data),
        4096,   /* rx buffer size */
        0,      /* id */
        NULL,   /* user */
        0       /* tx_packet_size */
    },
    LWS_PROTOCOL_LIST_TERM
};

/* --- Public API --- */

jig_server *jig_server_create(int port, jig_dispatcher_config *dispatcher, jig_app_info *app_info) {
    jig_server *server = calloc(1, sizeof(jig_server));
    if (!server) return NULL;

    server->port = port;
    server->dispatcher = dispatcher;
    server->app_info = app_info;
    server->stop_flag = 0;
    server->next_connection_id = 1;

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = port;
    info.protocols = protocols;
    info.options = 0;
    info.user = server;  /* retrievable via lws_context_user() in callbacks */

    server->lws_ctx = lws_create_context(&info);
    if (!server->lws_ctx) {
        free(server);
        return NULL;
    }

    return server;
}

int jig_server_start(jig_server *server) {
    if (!server || !server->lws_ctx) return -1;

    while (!server->stop_flag) {
        lws_service(server->lws_ctx, 100);
    }

    return 0;
}

void jig_server_stop(jig_server *server) {
    if (!server) return;
    server->stop_flag = 1;
    if (server->lws_ctx) {
        lws_cancel_service(server->lws_ctx);
    }
}

void jig_server_destroy(jig_server *server) {
    if (!server) return;
    if (server->lws_ctx) {
        lws_context_destroy(server->lws_ctx);
        server->lws_ctx = NULL;
    }
    free(server);
}
