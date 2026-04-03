#import "JigHandshakeHandler.h"

@interface JigHandshakeHandler ()
@property (nonatomic, copy) NSString *rnVersion;
@property (nonatomic, copy, nullable) NSString *expoVersion;
@property (nonatomic, copy) NSString *appName;
@property (nonatomic, copy) NSString *bundleId;
@end

@implementation JigHandshakeHandler

+ (NSInteger)protocolVersion { return 1; }
+ (NSString *)serverVersion { return @"0.1.0"; }

- (instancetype)initWithRnVersion:(NSString *)rnVersion
                     expoVersion:(NSString *)expoVersion {
    self = [super init];
    if (self) {
        _rnVersion = [rnVersion copy];
        _expoVersion = [expoVersion copy];
        _appName = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleName"] ?: @"Unknown";
        _bundleId = [NSBundle mainBundle].bundleIdentifier ?: @"unknown";
    }
    return self;
}

- (NSString *)makeServerHello {
    NSMutableDictionary *appDict = [@{
        @"name": self.appName,
        @"bundleId": self.bundleId,
        @"platform": @"ios",
        @"rn": self.rnVersion,
    } mutableCopy];
    if (self.expoVersion) {
        appDict[@"expo"] = self.expoVersion;
    }

    NSDictionary *params = @{
        @"protocol": @{
            @"name": @"jig",
            @"version": @([self.class protocolVersion]),
            @"min": @([self.class protocolVersion]),
            @"max": @([self.class protocolVersion]),
        },
        @"server": [self.class serverVersion],
        @"app": [appDict copy],
        @"commands": @[],
        @"domains": @[],
    };

    NSDictionary *message = @{
        @"jsonrpc": @"2.0",
        @"method": @"server.hello",
        @"params": params,
    };

    return [self jsonStringFromDictionary:message];
}

- (nullable NSString *)handleMessage:(NSString *)text {
    NSData *data = [text dataUsingEncoding:NSUTF8StringEncoding];
    if (!data) return nil;

    NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    if (![json isKindOfClass:[NSDictionary class]]) return nil;

    NSString *method = json[@"method"];
    if ([method isEqualToString:@"client.hello"]) {
        return [self handleClientHello:json];
    }

    return nil;
}

#pragma mark - Private

- (NSString *)handleClientHello:(NSDictionary *)json {
    NSInteger msgId = [json[@"id"] integerValue];
    NSDictionary *params = json[@"params"];
    NSDictionary *clientProtocol = params[@"protocol"];
    NSInteger requestedVersion = [clientProtocol[@"version"] integerValue];

    if (!params || !clientProtocol || requestedVersion == 0) {
        return [self makeErrorWithId:msgId code:-32600 message:@"Invalid client.hello params" data:nil];
    }

    NSInteger ours = [self.class protocolVersion];
    if (requestedVersion < ours || requestedVersion > ours) {
        NSDictionary *errorData = @{
            @"supported": @{ @"min": @(ours), @"max": @(ours) }
        };
        NSString *msg = [NSString stringWithFormat:
            @"Protocol version %ld not supported. Server supports version %ld.",
            (long)requestedVersion, (long)ours];
        return [self makeErrorWithId:msgId code:-32001 message:msg data:errorData];
    }

    NSString *sessionId = [NSString stringWithFormat:@"sess_%@",
        [[[NSUUID UUID] UUIDString] substringToIndex:8].lowercaseString];

    NSDictionary *result = @{
        @"sessionId": sessionId,
        @"negotiated": @{ @"protocol": @(requestedVersion) },
        @"enabled": @[],
        @"rejected": @[],
    };

    NSDictionary *response = @{
        @"jsonrpc": @"2.0",
        @"id": @(msgId),
        @"result": result,
    };

    return [self jsonStringFromDictionary:response];
}

- (NSString *)makeErrorWithId:(NSInteger)msgId
                         code:(NSInteger)code
                      message:(NSString *)message
                         data:(nullable NSDictionary *)data {
    NSMutableDictionary *error = [@{
        @"code": @(code),
        @"message": message,
    } mutableCopy];
    if (data) {
        error[@"data"] = data;
    }

    NSDictionary *response = @{
        @"jsonrpc": @"2.0",
        @"id": @(msgId),
        @"error": [error copy],
    };

    return [self jsonStringFromDictionary:response];
}

- (NSString *)jsonStringFromDictionary:(NSDictionary *)dict {
    NSData *data = [NSJSONSerialization dataWithJSONObject:dict
                                                  options:NSJSONWritingSortedKeys
                                                    error:nil];
    return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
}

@end
