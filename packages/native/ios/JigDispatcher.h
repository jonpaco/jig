// ios/JigDispatcher.h

#import <Foundation/Foundation.h>

@protocol JigHandler;
@protocol JigMiddleware;
@class JigSessionContext;

NS_ASSUME_NONNULL_BEGIN

@interface JigDispatcher : NSObject

/**
 * @param middlewares     Ordered validators run before dispatch
 * @param handlers       Command handlers — duplicate methods are a fatal error
 * @param supportedDomains  Domain names for server.hello and DomainGuard
 * @param queue          The WebSocket serial queue (for thread dispatch callbacks)
 */
- (instancetype)initWithMiddlewares:(NSArray<id<JigMiddleware>> *)middlewares
                           handlers:(NSArray<id<JigHandler>> *)handlers
                   supportedDomains:(NSArray<NSString *> *)supportedDomains
                              queue:(dispatch_queue_t)queue;

/** Send server.hello to a newly connected client */
- (void)handleOpen:(JigSessionContext *)context;

/** Parse, validate, route, and respond to an incoming message */
- (void)dispatch:(NSString *)text context:(JigSessionContext *)context;

@end

NS_ASSUME_NONNULL_END
