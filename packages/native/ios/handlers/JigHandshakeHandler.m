#import "JigHandshakeHandler.h"
#import "JigSessionContext.h"
#import "JigErrors.h"

static const NSInteger kProtocolVersion = 1;

@implementation JigHandshakeHandler

- (NSString *)method {
    return @"client.hello";
}

- (JigThreadTarget)threadTarget {
    return JigThreadTargetWebSocket;
}

- (id)handleParams:(NSDictionary *)params
           context:(JigSessionContext *)context
             error:(NSError **)error {

    NSDictionary *clientProtocol = params[@"protocol"];
    NSInteger requestedVersion = [clientProtocol[@"version"] integerValue];

    if (!params || !clientProtocol || requestedVersion == 0) {
        if (error) {
            *error = [JigErrors invalidParams:@"Invalid client.hello params"];
        }
        return nil;
    }

    if (requestedVersion < kProtocolVersion || requestedVersion > kProtocolVersion) {
        if (error) {
            *error = [JigErrors protocolVersionError:requestedVersion
                                           supported:@{
                                               @"min": @(kProtocolVersion),
                                               @"max": @(kProtocolVersion),
                                           }];
        }
        return nil;
    }

    NSString *sessionId = [NSString stringWithFormat:@"sess_%@",
        [[[NSUUID UUID] UUIDString] substringToIndex:8].lowercaseString];

    return @{
        @"sessionId": sessionId,
        @"negotiated": @{@"protocol": @(requestedVersion)},
        @"enabled": @[],
        @"rejected": @[],
    };
}

@end
