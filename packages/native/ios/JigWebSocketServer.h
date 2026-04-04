// ios/JigWebSocketServer.h

#import <Foundation/Foundation.h>
#import <Network/Network.h>

@class JigDispatcher;
@class JigAppInfo;

NS_ASSUME_NONNULL_BEGIN

@interface JigWebSocketServer : NSObject

- (instancetype)initWithPort:(uint16_t)port
                  dispatcher:(JigDispatcher *)dispatcher
                     appInfo:(JigAppInfo *)appInfo
                       queue:(dispatch_queue_t)queue;
- (void)start;
- (void)stop;

@end

NS_ASSUME_NONNULL_END
