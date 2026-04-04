#import <Foundation/Foundation.h>
#import "JigMiddleware.h"

NS_ASSUME_NONNULL_BEGIN

/** For *.enable / *.disable methods, checks that the domain is supported. */
@interface JigDomainGuard : NSObject <JigMiddleware>

- (instancetype)initWithSupportedDomains:(NSArray<NSString *> *)supportedDomains;

@end

NS_ASSUME_NONNULL_END
