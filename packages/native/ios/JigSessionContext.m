#import "JigSessionContext.h"

@implementation JigSessionContext

- (instancetype)initWithConnectionId:(NSInteger)connectionId
                             appInfo:(JigAppInfo *)appInfo {
    self = [super init];
    if (self) {
        _connectionId = connectionId;
        _appInfo = appInfo;
        _enabledDomains = [NSMutableSet new];
        _subscriptions = [NSMutableDictionary new];
    }
    return self;
}

@end
