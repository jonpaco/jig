#import "JigErrors.h"

NSString *const JigErrorDomain = @"JigErrorDomain";
NSString *const JigErrorDataKey = @"JigErrorData";

@implementation JigErrors

+ (NSError *)parseError:(NSString *)message {
    return [NSError errorWithDomain:JigErrorDomain
                               code:JigErrorCodeParseError
                           userInfo:@{NSLocalizedDescriptionKey: message}];
}

+ (NSError *)invalidRequest:(NSString *)message {
    return [NSError errorWithDomain:JigErrorDomain
                               code:JigErrorCodeInvalidRequest
                           userInfo:@{NSLocalizedDescriptionKey: message}];
}

+ (NSError *)methodNotFound:(NSString *)method {
    NSString *msg = [NSString stringWithFormat:@"Method '%@' not found", method];
    return [NSError errorWithDomain:JigErrorDomain
                               code:JigErrorCodeMethodNotFound
                           userInfo:@{NSLocalizedDescriptionKey: msg}];
}

+ (NSError *)invalidParams:(NSString *)message {
    return [NSError errorWithDomain:JigErrorDomain
                               code:JigErrorCodeInvalidParams
                           userInfo:@{NSLocalizedDescriptionKey: message}];
}

+ (NSError *)internalError:(NSString *)message {
    return [NSError errorWithDomain:JigErrorDomain
                               code:JigErrorCodeInternalError
                           userInfo:@{NSLocalizedDescriptionKey: message}];
}

+ (NSError *)handshakeRequired {
    return [NSError errorWithDomain:JigErrorDomain
                               code:JigErrorCodeHandshakeRequired
                           userInfo:@{
                               NSLocalizedDescriptionKey:
                                   @"Handshake required before sending commands",
                           }];
}

+ (NSError *)protocolVersionError:(NSInteger)requested
                        supported:(NSDictionary *)supported {
    NSString *msg = [NSString stringWithFormat:
        @"Protocol version %ld not supported. Server supports version %ld.",
        (long)requested, (long)[supported[@"min"] integerValue]];
    return [NSError errorWithDomain:JigErrorDomain
                               code:JigErrorCodeProtocolVersion
                           userInfo:@{
                               NSLocalizedDescriptionKey: msg,
                               JigErrorDataKey: @{@"supported": supported},
                           }];
}

+ (NSError *)elementNotFound:(NSString *)message data:(NSDictionary *)data {
    NSMutableDictionary *userInfo = [@{
        NSLocalizedDescriptionKey: message,
    } mutableCopy];
    if (data) {
        userInfo[JigErrorDataKey] = data;
    }
    return [NSError errorWithDomain:JigErrorDomain
                               code:JigErrorCodeElementNotFound
                           userInfo:[userInfo copy]];
}

+ (NSError *)timeout:(NSString *)message {
    return [NSError errorWithDomain:JigErrorDomain
                               code:JigErrorCodeTimeout
                           userInfo:@{NSLocalizedDescriptionKey: message}];
}

+ (NSError *)invalidSelector:(NSString *)message {
    return [NSError errorWithDomain:JigErrorDomain
                               code:JigErrorCodeInvalidSelector
                           userInfo:@{NSLocalizedDescriptionKey: message}];
}

+ (NSError *)unsupportedDomain:(NSString *)domain {
    NSString *msg = [NSString stringWithFormat:@"Domain '%@' is not supported", domain];
    return [NSError errorWithDomain:JigErrorDomain
                               code:JigErrorCodeUnsupportedDomain
                           userInfo:@{NSLocalizedDescriptionKey: msg}];
}

+ (NSError *)domainNotEnabled:(NSString *)domain {
    NSString *msg = [NSString stringWithFormat:
        @"Domain '%@' must be enabled before use", domain];
    return [NSError errorWithDomain:JigErrorDomain
                               code:JigErrorCodeDomainNotEnabled
                           userInfo:@{NSLocalizedDescriptionKey: msg}];
}

@end
