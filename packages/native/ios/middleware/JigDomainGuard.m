#import "JigDomainGuard.h"
#import "JigSessionContext.h"
#import "JigErrors.h"

@interface JigDomainGuard ()
@property (nonatomic, strong) NSSet<NSString *> *supportedDomains;
@end

@implementation JigDomainGuard

- (instancetype)initWithSupportedDomains:(NSArray<NSString *> *)supportedDomains {
    self = [super init];
    if (self) {
        _supportedDomains = [NSSet setWithArray:supportedDomains];
    }
    return self;
}

- (BOOL)validateMethod:(NSString *)method
                params:(NSDictionary *)params
               context:(JigSessionContext *)context
                 error:(NSError **)error {
    if ([method hasSuffix:@".enable"] || [method hasSuffix:@".disable"]) {
        NSString *domain = [method componentsSeparatedByString:@"."].firstObject;
        if (![self.supportedDomains containsObject:domain]) {
            if (error) {
                *error = [JigErrors unsupportedDomain:domain];
            }
            return NO;
        }
    }
    return YES;
}

@end
