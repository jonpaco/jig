#import <Foundation/Foundation.h>
#import <Network/Network.h>

@class JigHandshakeHandler;

NS_ASSUME_NONNULL_BEGIN

@interface JigWebSocketServer : NSObject

- (instancetype)initWithPort:(uint16_t)port
                     handler:(JigHandshakeHandler *)handler;
- (void)start;
- (void)stop;

@end

NS_ASSUME_NONNULL_END
