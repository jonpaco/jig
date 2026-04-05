// ios/JigBridge.m
//
// Obj-C bridge — wraps C core server lifecycle for Swift (JigModule).

#import "JigBridge.h"
#include "JigBootstrap.h"

static jig_server *_server = NULL;

@implementation JigBridge

+ (void)startServerWithName:(NSString *)name
                   bundleId:(NSString *)bundleId
                  rnVersion:(NSString *)rnVersion
                expoVersion:(NSString *)expoVersion
                       port:(int)port {
    if (_server != NULL) return;

    _server = jig_bootstrap_server(
        [name UTF8String],
        [bundleId UTF8String],
        [rnVersion UTF8String],
        expoVersion ? [expoVersion UTF8String] : NULL,
        port
    );
}

+ (void)stopServer {
    if (_server == NULL) return;
    jig_bootstrap_stop(_server);
    _server = NULL;
}

+ (BOOL)isRunning {
    return _server != NULL;
}

@end
