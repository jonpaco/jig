// ios/JigBootstrap.m
//
// Shared server bootstrap implementation.

#import <Foundation/Foundation.h>

#include "JigBootstrap.h"
#include "../core/jig_server.h"
#include "../core/jig_dispatcher.h"
#include "../core/jig_platform.h"
#include "../core/jig_app_info.h"
#include "../core/handlers/jig_handshake.h"
#include "../core/middleware/jig_handshake_gate.h"
#include "../core/middleware/jig_domain_guard.h"
#include "../core/jig_jsbridge.h"
#include "JigJSInjector.h"

// Forward declare the screenshot handler create function (defined in JigScreenshotShim.m)
extern jig_handler *jig_screenshot_handler_create(void);

extern jig_handler *jig_find_element_handler_create(jig_jsbridge *bridge);
extern jig_handler *jig_find_elements_handler_create(jig_jsbridge *bridge);

// Platform ops: logging via NSLog (format-string-safe)
static void ios_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char *buf = NULL;
    vasprintf(&buf, fmt, args);
    va_end(args);
    if (buf) {
        NSLog(@"%s", buf);
        free(buf);
    }
}

static void ios_run_on_main_thread(void (*callback)(void *ctx), void *ctx) {
    dispatch_async(dispatch_get_main_queue(), ^{
        callback(ctx);
    });
}

jig_server *jig_bootstrap_server(const char *name, const char *bundle_id,
                                  const char *rn_version, const char *expo_version,
                                  int port) {
    // 1. Set platform ops
    static jig_platform_ops ops = {
        .screenshot = NULL,
        .get_app_info = NULL,
        .log = ios_log,
        .run_on_main_thread = ios_run_on_main_thread,
    };
    jig_platform_set_ops(&ops);

    // 2. Create app info
    jig_app_info *app_info = jig_app_info_create(
        name, bundle_id, "ios", rn_version, expo_version
    );

    // 3. Create JS bridge (manages internal JS client for fiber data)
    static jig_jsbridge *jsbridge = NULL;
    jsbridge = jig_jsbridge_create();
    jig_handler **bridge_handlers = jig_jsbridge_get_handlers(jsbridge);

    // 4. Create handlers — must be static since dispatcher stores pointers
    static jig_handler *handlers[6];
    handlers[0] = jig_handshake_create();
    handlers[1] = jig_screenshot_handler_create();
    handlers[2] = jig_find_element_handler_create(jsbridge);
    handlers[3] = jig_find_elements_handler_create(jsbridge);
    handlers[4] = bridge_handlers[0]; /* jig.internal.ready */
    handlers[5] = bridge_handlers[1]; /* jig.internal.fiberData */

    static jig_middleware middlewares[2];
    middlewares[0] = jig_handshake_gate_create();
    middlewares[1] = jig_domain_guard_create(NULL, 0);

    // 5. Create dispatcher
    jig_dispatcher_config *dispatcher = jig_dispatcher_create(
        middlewares, 2,
        handlers, 6,
        NULL, 0
    );

    // 6. Create and start server on background thread
    jig_server *server = jig_server_create(port, dispatcher, app_info);
    dispatch_async(dispatch_queue_create("jig.server", NULL), ^{
        jig_server_start(server);
    });

    // 7. Start JS injector (listens for RN bridge, injects fiber walker)
    jig_ios_start_js_injector();

    NSLog(@"[Jig] Server started on port %d for %s (%s)", port, name, bundle_id);
    return server;
}

void jig_bootstrap_stop(jig_server *server) {
    if (server == NULL) return;
    jig_server_stop(server);
    jig_server_destroy(server);
}
