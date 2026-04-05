// ios/JigBridge.h
//
// Obj-C bridge — exposes C core functions to Swift (JigModule).

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface JigBridge : NSObject

+ (void)startServerWithName:(NSString *)name
                   bundleId:(NSString *)bundleId
                  rnVersion:(NSString *)rnVersion
                expoVersion:(NSString * _Nullable)expoVersion
                       port:(int)port;

+ (void)stopServer;
+ (BOOL)isRunning;

@end

NS_ASSUME_NONNULL_END
