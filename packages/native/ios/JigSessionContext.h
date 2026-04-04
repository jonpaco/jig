#import <Foundation/Foundation.h>

@class JigAppInfo;

typedef void (^JigSendBlock)(NSString *text);

NS_ASSUME_NONNULL_BEGIN

@interface JigSessionContext : NSObject

@property (nonatomic, readonly) NSInteger connectionId;
@property (nonatomic, readonly, strong) JigAppInfo *appInfo;
@property (nonatomic, copy) JigSendBlock sendText;
@property (nonatomic, copy, nullable) dispatch_block_t cancel;

/** Set once during handshake — nil before handshake completes */
@property (nonatomic, copy, nullable) NSString *sessionId;
/** Set once during handshake */
@property (nonatomic, assign) NSInteger negotiatedVersion;
/** Domains currently enabled for this connection */
@property (nonatomic, strong) NSMutableSet<NSString *> *enabledDomains;
/** Per-domain subscription options */
@property (nonatomic, strong) NSMutableDictionary<NSString *, NSDictionary *> *subscriptions;

- (instancetype)initWithConnectionId:(NSInteger)connectionId
                             appInfo:(JigAppInfo *)appInfo;

@end

NS_ASSUME_NONNULL_END
