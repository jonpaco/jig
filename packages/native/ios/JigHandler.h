// ios/JigHandler.h

#import <Foundation/Foundation.h>

@class JigSessionContext;

typedef NS_ENUM(NSInteger, JigThreadTarget) {
    JigThreadTargetWebSocket = 0,
    JigThreadTargetMain = 1,
};

NS_ASSUME_NONNULL_BEGIN

@protocol JigHandler <NSObject>

/** The JSON-RPC method this handler responds to (e.g., "client.hello") */
@property (nonatomic, readonly) NSString *method;

/** Where this handler must execute */
@property (nonatomic, readonly) JigThreadTarget threadTarget;

/**
 * Handle the command.
 * @param params  The parsed "params" dictionary from the JSON-RPC message (may be nil)
 * @param context The session context for this connection
 * @param error   Set on failure — the dispatcher formats the JSON-RPC error response
 * @return The result object for the JSON-RPC response, or nil on error
 */
- (nullable id)handleParams:(nullable NSDictionary *)params
                    context:(JigSessionContext *)context
                      error:(NSError **)error;

@end

NS_ASSUME_NONNULL_END
