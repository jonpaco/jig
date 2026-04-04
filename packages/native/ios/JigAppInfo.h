#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface JigAppInfo : NSObject

@property (nonatomic, readonly, copy) NSString *name;
@property (nonatomic, readonly, copy) NSString *bundleId;
@property (nonatomic, readonly, copy) NSString *platform;
@property (nonatomic, readonly, copy) NSString *rnVersion;
@property (nonatomic, readonly, copy, nullable) NSString *expoVersion;

- (instancetype)initWithName:(NSString *)name
                    bundleId:(NSString *)bundleId
                    platform:(NSString *)platform
                   rnVersion:(NSString *)rnVersion
                 expoVersion:(nullable NSString *)expoVersion;

- (NSDictionary *)toDictionary;

@end

NS_ASSUME_NONNULL_END
