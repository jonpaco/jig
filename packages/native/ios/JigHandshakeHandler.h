#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface JigHandshakeHandler : NSObject

@property (class, readonly) NSInteger protocolVersion;
@property (class, readonly) NSString *serverVersion;

- (instancetype)initWithRnVersion:(NSString *)rnVersion
                     expoVersion:(nullable NSString *)expoVersion;

- (NSString *)makeServerHello;
- (nullable NSString *)handleMessage:(NSString *)text;

@end

NS_ASSUME_NONNULL_END
