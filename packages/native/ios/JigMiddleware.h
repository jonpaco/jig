// ios/JigMiddleware.h

#import <Foundation/Foundation.h>

@class JigSessionContext;

NS_ASSUME_NONNULL_BEGIN

@protocol JigMiddleware <NSObject>

/**
 * Validate the incoming message. Return YES to continue the chain, NO to reject.
 * @param method  The JSON-RPC method name
 * @param params  The parsed params (may be nil)
 * @param context The session context
 * @param error   Set on rejection — the dispatcher formats the JSON-RPC error response
 */
- (BOOL)validateMethod:(NSString *)method
                params:(nullable NSDictionary *)params
               context:(JigSessionContext *)context
                 error:(NSError **)error;

@end

NS_ASSUME_NONNULL_END
