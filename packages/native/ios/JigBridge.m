// ios/JigBridge.m
//
// Obj-C bridge — wraps C core server lifecycle for Swift (JigModule).

#import "JigBridge.h"

#include "../core/jig_server.h"
#include "../core/jig_dispatcher.h"
#include "../core/jig_platform.h"
#include "../core/jig_app_info.h"
#include "../core/handlers/jig_handshake.h"
#include "../core/middleware/jig_handshake_gate.h"
#include "../core/middleware/jig_domain_guard.h"

// Forward declare the screenshot handler create function (defined in JigScreenshotShim.m)
extern jig_handler *jig_screenshot_handler_create(void);

static jig_server *_server = NULL;

// Platform ops: logging via NSLog
static void ios_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    NSString *nsFmt = [NSString stringWithUTF8String:fmt];
    NSLogv(nsFmt, args);
    va_end(args);
}

@implementation JigBridge

+ (void)startServerWithName:(NSString *)name
                   bundleId:(NSString *)bundleId
                  rnVersion:(NSString *)rnVersion
                expoVersion:(NSString *)expoVersion
                       port:(int)port {
    if (_server != NULL) return;

    // 1. Set platform ops
    jig_platform_ops ops = {
        .screenshot = NULL,
        .get_app_info = NULL,
        .log = ios_log,
    };
    jig_platform_set_ops(&ops);

    // 2. Create app info
    jig_app_info *app_info = jig_app_info_create(
        [name UTF8String],
        [bundleId UTF8String],
        "ios",
        [rnVersion UTF8String],
        expoVersion ? [expoVersion UTF8String] : NULL
    );

    // 3. Create handlers and middleware
    jig_handler *handlers[] = {
        jig_handshake_create(),
        jig_screenshot_handler_create(),
    };
    jig_middleware middlewares[] = {
        jig_handshake_gate_create(),
        jig_domain_guard_create(NULL, 0),
    };

    // 4. Create dispatcher
    jig_dispatcher_config *dispatcher = jig_dispatcher_create(
        middlewares, 2,
        handlers, 2,
        NULL, 0
    );

    // 5. Create and start server on background thread
    _server = jig_server_create(port, dispatcher, app_info);
    dispatch_async(dispatch_queue_create("jig.server", NULL), ^{
        jig_server_start(_server);
    });

    NSLog(@"[Jig] Server started on port %d for %@ (%@)", port, name, bundleId);
}

+ (void)stopServer {
    if (_server == NULL) return;
    jig_server_stop(_server);
    jig_server_destroy(_server);
    _server = NULL;
}

+ (BOOL)isRunning {
    return _server != NULL;
}

@end
