#include "jig_jsbridge.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

struct jig_jsbridge {
    jig_session *js_session;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    cJSON *pending_fibers;
    int fibers_ready;
    jig_handler *handlers[JIG_JSBRIDGE_HANDLER_COUNT];
};

static cJSON *ready_handle(jig_handler *self, cJSON *params,
                           jig_session *session, jig_error **err) {
    (void)params; (void)err;
    jig_jsbridge *bridge = (jig_jsbridge *)self->user_data;
    pthread_mutex_lock(&bridge->mutex);
    bridge->js_session = session;
    pthread_mutex_unlock(&bridge->mutex);
    cJSON *result = cJSON_CreateObject();
    cJSON_AddBoolToObject(result, "ok", 1);
    return result;
}

static cJSON *fiber_data_handle(jig_handler *self, cJSON *params,
                                jig_session *session, jig_error **err) {
    (void)session; (void)err;
    jig_jsbridge *bridge = (jig_jsbridge *)self->user_data;
    pthread_mutex_lock(&bridge->mutex);
    if (bridge->pending_fibers) cJSON_Delete(bridge->pending_fibers);
    cJSON *fibers = cJSON_GetObjectItemCaseSensitive(params, "fibers");
    bridge->pending_fibers = fibers ? cJSON_Duplicate(fibers, 1) : cJSON_CreateArray();
    bridge->fibers_ready = 1;
    pthread_cond_signal(&bridge->cond);
    pthread_mutex_unlock(&bridge->mutex);
    return NULL;
}

jig_jsbridge *jig_jsbridge_create(void) {
    jig_jsbridge *b = calloc(1, sizeof(jig_jsbridge));
    if (!b) return NULL;
    pthread_mutex_init(&b->mutex, NULL);
    pthread_cond_init(&b->cond, NULL);
    b->handlers[0] = calloc(1, sizeof(jig_handler));
    b->handlers[0]->method = "jig.internal.ready";
    b->handlers[0]->thread_target = JIG_THREAD_WEBSOCKET;
    b->handlers[0]->handle = ready_handle;
    b->handlers[0]->user_data = b;
    b->handlers[1] = calloc(1, sizeof(jig_handler));
    b->handlers[1]->method = "jig.internal.fiberData";
    b->handlers[1]->thread_target = JIG_THREAD_WEBSOCKET;
    b->handlers[1]->handle = fiber_data_handle;
    b->handlers[1]->user_data = b;
    return b;
}

void jig_jsbridge_destroy(jig_jsbridge *bridge) {
    if (!bridge) return;
    if (bridge->pending_fibers) cJSON_Delete(bridge->pending_fibers);
    free(bridge->handlers[0]);
    free(bridge->handlers[1]);
    pthread_mutex_destroy(&bridge->mutex);
    pthread_cond_destroy(&bridge->cond);
    free(bridge);
}

jig_handler **jig_jsbridge_get_handlers(jig_jsbridge *bridge) {
    return bridge->handlers;
}

cJSON *jig_jsbridge_walk_fibers(jig_jsbridge *bridge, int timeout_ms) {
    pthread_mutex_lock(&bridge->mutex);
    if (!bridge->js_session) {
        pthread_mutex_unlock(&bridge->mutex);
        return NULL;
    }
    if (bridge->pending_fibers) {
        cJSON_Delete(bridge->pending_fibers);
        bridge->pending_fibers = NULL;
    }
    bridge->fibers_ready = 0;
    const char *msg = "{\"jsonrpc\":\"2.0\",\"method\":\"jig.internal.walkFibers\",\"params\":{}}";
    bridge->js_session->send_text(msg, bridge->js_session->send_user_data);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long ns = (long)timeout_ms * 1000000L;
    ts.tv_nsec += ns;
    ts.tv_sec += ts.tv_nsec / 1000000000L;
    ts.tv_nsec %= 1000000000L;
    while (!bridge->fibers_ready) {
        int rc = pthread_cond_timedwait(&bridge->cond, &bridge->mutex, &ts);
        if (rc != 0) break;
    }
    cJSON *result = bridge->pending_fibers;
    bridge->pending_fibers = NULL;
    bridge->fibers_ready = 0;
    pthread_mutex_unlock(&bridge->mutex);
    return result;
}

void jig_jsbridge_on_disconnect(jig_jsbridge *bridge, jig_session *session) {
    if (!bridge) return;
    pthread_mutex_lock(&bridge->mutex);
    if (bridge->js_session == session) bridge->js_session = NULL;
    pthread_mutex_unlock(&bridge->mutex);
}
