#import <Foundation/Foundation.h>
#import "JigMiddleware.h"

NS_ASSUME_NONNULL_BEGIN

/** Rejects any command received before handshake completes (except client.hello). */
@interface JigHandshakeGate : NSObject <JigMiddleware>
@end

NS_ASSUME_NONNULL_END
