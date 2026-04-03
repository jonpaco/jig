#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface JigWebSocketServer : NSObject

- (instancetype)initWithPort:(uint16_t)port
                   rnVersion:(NSString *)rnVersion
                expoVersion:(nullable NSString *)expoVersion;
- (void)start;
- (void)stop;

@end

NS_ASSUME_NONNULL_END
