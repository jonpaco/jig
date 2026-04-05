// ios/JigStandaloneInit.m
//
// Entry point for standalone Jig.framework injection.
// Loaded via DYLD_INSERT_LIBRARIES — runs before main().

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include "JigBootstrap.h"

static jig_server *_jigServer = NULL;

__attribute__((constructor))
static void JigStandaloneInit(void) {
    NSBundle *mainBundle = [NSBundle mainBundle];
    NSDictionary *info = mainBundle.infoDictionary;

    NSString *appName = info[@"CFBundleDisplayName"]
                     ?: info[@"CFBundleName"]
                     ?: @"Unknown";
    NSString *bundleId = mainBundle.bundleIdentifier ?: @"unknown";

    _jigServer = jig_bootstrap_server(
        [appName UTF8String],
        [bundleId UTF8String],
        "unknown",
        NULL,
        4042
    );
}
