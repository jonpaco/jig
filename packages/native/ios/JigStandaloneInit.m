// ios/JigStandaloneInit.m
//
// Entry point for standalone Jig.framework injection.
// Loaded via DYLD_INSERT_LIBRARIES — runs before main().

#import <Foundation/Foundation.h>
#import "JigWebSocketServer.h"
#import "JigDispatcher.h"
#import "JigAppInfo.h"
#import "JigSessionContext.h"
#import "handlers/JigHandshakeHandler.h"
#import "handlers/JigScreenshotHandler.h"
#import "middleware/JigHandshakeGate.h"
#import "middleware/JigDomainGuard.h"

static JigWebSocketServer *_jigServer = nil;

__attribute__((constructor))
static void JigStandaloneInit(void) {
    NSBundle *mainBundle = [NSBundle mainBundle];
    NSDictionary *info = mainBundle.infoDictionary;

    NSString *appName = info[@"CFBundleDisplayName"]
                     ?: info[@"CFBundleName"]
                     ?: @"Unknown";
    NSString *bundleId = mainBundle.bundleIdentifier ?: @"unknown";

    JigAppInfo *appInfo = [[JigAppInfo alloc] initWithName:appName
                                                  bundleId:bundleId
                                                  platform:@"ios"
                                                 rnVersion:@"unknown"
                                               expoVersion:nil];

    NSArray<NSString *> *supportedDomains = @[];

    NSArray<id<JigMiddleware>> *middlewares = @[
        [[JigHandshakeGate alloc] init],
        [[JigDomainGuard alloc] initWithSupportedDomains:supportedDomains],
    ];
    NSArray<id<JigHandler>> *handlers = @[
        [[JigHandshakeHandler alloc] init],
        [[JigScreenshotHandler alloc] init],
    ];

    dispatch_queue_t queue = dispatch_queue_create("jig.websocket", DISPATCH_QUEUE_SERIAL);

    JigDispatcher *dispatcher = [[JigDispatcher alloc] initWithMiddlewares:middlewares
                                                                 handlers:handlers
                                                         supportedDomains:supportedDomains
                                                                    queue:queue];

    _jigServer = [[JigWebSocketServer alloc] initWithPort:4042
                                               dispatcher:dispatcher
                                                  appInfo:appInfo
                                                    queue:queue];
    [_jigServer start];

    NSLog(@"[Jig] Standalone server started on port 4042 for %@ (%@)", appName, bundleId);
}
