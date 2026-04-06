#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#import <objc/message.h>

#include "JigJSInjector.h"

static NSString *load_fiber_walker_js(void) {
    NSBundle *bundle = [NSBundle bundleForClass:NSClassFromString(@"JigModule")];
    if (!bundle) {
        bundle = [NSBundle mainBundle];
    }

    NSString *path = [bundle pathForResource:@"fiber-walker" ofType:@"js"];
    if (path) {
        NSError *error = nil;
        NSString *js = [NSString stringWithContentsOfFile:path
                                                encoding:NSUTF8StringEncoding
                                                   error:&error];
        if (js) return js;
        NSLog(@"[Jig] Failed to read fiber-walker.js: %@", error);
    }

    NSLog(@"[Jig] fiber-walker.js not found in bundle, using embedded fallback");
    return nil;
}

static void inject_js(id bridge) {
    NSString *jsCode = load_fiber_walker_js();
    if (!jsCode) {
        NSLog(@"[Jig] Cannot inject fiber walker: JS payload not available");
        return;
    }

    SEL batchedSel = NSSelectorFromString(@"batchedBridge");
    if (![bridge respondsToSelector:batchedSel]) {
        NSLog(@"[Jig] Bridge does not respond to batchedBridge");
        return;
    }

    id batchedBridge = ((id (*)(id, SEL))objc_msgSend)(bridge, batchedSel);
    if (!batchedBridge) {
        NSLog(@"[Jig] batchedBridge is nil");
        return;
    }

    SEL execSel = NSSelectorFromString(@"executeApplicationScript:url:async:");
    if ([batchedBridge respondsToSelector:execSel]) {
        NSData *scriptData = [jsCode dataUsingEncoding:NSUTF8StringEncoding];
        NSURL *url = [NSURL URLWithString:@"jig://fiber-walker.js"];
        BOOL async = YES;

        NSMethodSignature *sig = [batchedBridge methodSignatureForSelector:execSel];
        NSInvocation *inv = [NSInvocation invocationWithMethodSignature:sig];
        [inv setSelector:execSel];
        [inv setArgument:&scriptData atIndex:2];
        [inv setArgument:&url atIndex:3];
        [inv setArgument:&async atIndex:4];
        [inv invokeWithTarget:batchedBridge];

        NSLog(@"[Jig] Fiber walker JS injected via executeApplicationScript");
        return;
    }

    SEL execSel2 = NSSelectorFromString(@"executeSourceCode:sync:");
    if ([batchedBridge respondsToSelector:execSel2]) {
        NSData *scriptData = [jsCode dataUsingEncoding:NSUTF8StringEncoding];
        BOOL sync = NO;

        NSMethodSignature *sig = [batchedBridge methodSignatureForSelector:execSel2];
        NSInvocation *inv = [NSInvocation invocationWithMethodSignature:sig];
        [inv setSelector:execSel2];
        [inv setArgument:&scriptData atIndex:2];
        [inv setArgument:&sync atIndex:3];
        [inv invokeWithTarget:batchedBridge];

        NSLog(@"[Jig] Fiber walker JS injected via executeSourceCode");
        return;
    }

    NSLog(@"[Jig] No JS execution method found on bridge — component selectors unavailable");
}

void jig_ios_start_js_injector(void) {
    [[NSNotificationCenter defaultCenter]
        addObserverForName:@"RCTJavaScriptDidLoadNotification"
                    object:nil
                     queue:nil
                usingBlock:^(NSNotification *note) {
        NSLog(@"[Jig] RCTJavaScriptDidLoadNotification received");
        id bridge = note.object;
        if (bridge) {
            inject_js(bridge);
        } else {
            NSLog(@"[Jig] Notification had no bridge object");
        }
    }];
    NSLog(@"[Jig] JS injector listening for React Native bridge");
}
