#import "JigAppInfo.h"

@implementation JigAppInfo

- (instancetype)initWithName:(NSString *)name
                    bundleId:(NSString *)bundleId
                    platform:(NSString *)platform
                   rnVersion:(NSString *)rnVersion
                 expoVersion:(NSString *)expoVersion {
    self = [super init];
    if (self) {
        _name = [name copy];
        _bundleId = [bundleId copy];
        _platform = [platform copy];
        _rnVersion = [rnVersion copy];
        _expoVersion = [expoVersion copy];
    }
    return self;
}

- (NSDictionary *)toDictionary {
    NSMutableDictionary *dict = [@{
        @"name": self.name,
        @"bundleId": self.bundleId,
        @"platform": self.platform,
        @"rn": self.rnVersion,
    } mutableCopy];
    if (self.expoVersion) {
        dict[@"expo"] = self.expoVersion;
    }
    return [dict copy];
}

@end
