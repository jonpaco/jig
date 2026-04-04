// ios/JigDispatcher.m

#import "JigDispatcher.h"
#import "JigHandler.h"
#import "JigMiddleware.h"
#import "JigSessionContext.h"
#import "JigAppInfo.h"
#import "JigErrors.h"

static const NSInteger kProtocolVersion = 1;
static NSString *const kServerVersion = @"0.1.0";

@interface JigDispatcher ()
@property (nonatomic, strong) NSArray<id<JigMiddleware>> *middlewares;
@property (nonatomic, strong) NSDictionary<NSString *, id<JigHandler>> *handlerMap;
@property (nonatomic, strong) NSArray<NSString *> *supportedDomains;
@property (nonatomic, strong) NSArray<NSString *> *commandNames;
@property (nonatomic, strong) dispatch_queue_t queue;
@end

@implementation JigDispatcher

- (instancetype)initWithMiddlewares:(NSArray<id<JigMiddleware>> *)middlewares
                           handlers:(NSArray<id<JigHandler>> *)handlers
                   supportedDomains:(NSArray<NSString *> *)supportedDomains
                              queue:(dispatch_queue_t)queue {
    self = [super init];
    if (self) {
        _middlewares = middlewares;
        _supportedDomains = supportedDomains;
        _queue = queue;

        NSMutableDictionary *map = [NSMutableDictionary new];
        NSMutableArray *commands = [NSMutableArray new];
        for (id<JigHandler> handler in handlers) {
            NSString *method = handler.method;
            NSAssert(map[method] == nil,
                     @"[Jig] Duplicate handler for method: %@", method);
            map[method] = handler;
            if (![method isEqualToString:@"client.hello"]) {
                [commands addObject:method];
            }
        }
        _handlerMap = [map copy];
        _commandNames = [commands copy];
    }
    return self;
}

#pragma mark - Connection lifecycle

- (void)handleOpen:(JigSessionContext *)context {
    NSDictionary *params = @{
        @"protocol": @{
            @"name": @"jig",
            @"version": @(kProtocolVersion),
            @"min": @(kProtocolVersion),
            @"max": @(kProtocolVersion),
        },
        @"server": kServerVersion,
        @"app": [context.appInfo toDictionary],
        @"commands": self.commandNames,
        @"domains": self.supportedDomains,
    };

    NSDictionary *message = @{
        @"jsonrpc": @"2.0",
        @"method": @"server.hello",
        @"params": params,
    };

    context.sendText([self jsonStringFromDictionary:message]);
}

#pragma mark - Dispatch

- (void)dispatch:(NSString *)text context:(JigSessionContext *)context {
    // 1. Parse JSON
    NSData *data = [text dataUsingEncoding:NSUTF8StringEncoding];
    if (!data) {
        [self sendParseError:@"Invalid UTF-8" context:context];
        return;
    }

    NSError *parseErr = nil;
    id parsed = [NSJSONSerialization JSONObjectWithData:data options:0 error:&parseErr];
    if (parseErr || ![parsed isKindOfClass:[NSDictionary class]]) {
        [self sendParseError:@"Malformed JSON" context:context];
        return;
    }
    NSDictionary *json = (NSDictionary *)parsed;

    // 2. Extract method, id, params
    NSString *jsonrpc = json[@"jsonrpc"];
    NSString *method = json[@"method"];
    if (![jsonrpc isEqualToString:@"2.0"] || ![method isKindOfClass:[NSString class]]) {
        [self sendErrorWithCode:JigErrorCodeInvalidRequest
                        message:@"Missing or invalid 'jsonrpc' or 'method' field"
                           data:nil
                             id:json[@"id"]
                        context:context];
        return;
    }

    NSNumber *msgId = json[@"id"];  // nil for notifications
    NSDictionary *params = json[@"params"];
    BOOL isCommand = (msgId != nil);

    // 3. Run middleware chain
    for (id<JigMiddleware> middleware in self.middlewares) {
        NSError *mwError = nil;
        BOOL ok = [middleware validateMethod:method
                                     params:params
                                    context:context
                                      error:&mwError];
        if (!ok && mwError) {
            if (isCommand) {
                [self sendNSError:mwError id:msgId context:context];
            } else {
                NSLog(@"[Jig] Middleware rejected notification '%@': %@",
                      method, mwError.localizedDescription);
            }
            return;
        }
    }

    // 4. Look up handler
    id<JigHandler> handler = self.handlerMap[method];
    if (!handler) {
        if (isCommand) {
            [self sendErrorWithCode:JigErrorCodeMethodNotFound
                            message:[NSString stringWithFormat:
                                     @"Method '%@' not found", method]
                               data:nil
                                 id:msgId
                            context:context];
        } else {
            NSLog(@"[Jig] No handler for notification '%@'", method);
        }
        return;
    }

    // 5. Dispatch to handler's declared thread
    switch (handler.threadTarget) {
        case JigThreadTargetWebSocket:
            [self executeHandler:handler
                          params:params
                         context:context
                          method:method
                           msgId:msgId
                       isCommand:isCommand];
            break;

        case JigThreadTargetMain:
            dispatch_async(dispatch_get_main_queue(), ^{
                [self executeHandler:handler
                              params:params
                             context:context
                              method:method
                               msgId:msgId
                           isCommand:isCommand];
            });
            break;
    }
}

#pragma mark - Handler execution

- (void)executeHandler:(id<JigHandler>)handler
                params:(NSDictionary *)params
               context:(JigSessionContext *)context
                method:(NSString *)method
                 msgId:(NSNumber *)msgId
             isCommand:(BOOL)isCommand {

    NSError *handlerError = nil;
    id result = [handler handleParams:params context:context error:&handlerError];

    // Response must be sent on the WebSocket queue
    void (^respond)(void) = ^{
        if (handlerError) {
            if (isCommand) {
                [self sendNSError:handlerError id:msgId context:context];
            } else {
                NSLog(@"[Jig] Handler error for '%@': %@",
                      method, handlerError.localizedDescription);
            }
            return;
        }

        // Special case: update session context after successful handshake
        if ([method isEqualToString:@"client.hello"] &&
            [result isKindOfClass:[NSDictionary class]]) {
            NSDictionary *resultDict = (NSDictionary *)result;
            context.sessionId = resultDict[@"sessionId"];
            context.negotiatedVersion =
                [resultDict[@"negotiated"][@"protocol"] integerValue];
        }

        if (isCommand) {
            [self sendResult:result id:msgId context:context];
        }
    };

    if (handler.threadTarget == JigThreadTargetWebSocket) {
        respond();
    } else {
        dispatch_async(self.queue, respond);
    }
}

#pragma mark - Response formatting

- (void)sendResult:(id)result
                id:(NSNumber *)msgId
           context:(JigSessionContext *)context {
    NSDictionary *response = @{
        @"jsonrpc": @"2.0",
        @"id": msgId,
        @"result": result ?: [NSNull null],
    };
    context.sendText([self jsonStringFromDictionary:response]);
}

- (void)sendNSError:(NSError *)error
                 id:(NSNumber *)msgId
            context:(JigSessionContext *)context {
    NSDictionary *errorData = error.userInfo[JigErrorDataKey];
    [self sendErrorWithCode:error.code
                    message:error.localizedDescription
                       data:errorData
                         id:msgId
                    context:context];
}

- (void)sendErrorWithCode:(NSInteger)code
                  message:(NSString *)message
                     data:(NSDictionary *)data
                       id:(NSNumber *)msgId
                  context:(JigSessionContext *)context {
    // Use NSNull for id if nil (parse errors / invalid requests)
    id responseId = msgId ?: [NSNull null];

    NSMutableDictionary *errorDict = [@{
        @"code": @(code),
        @"message": message,
    } mutableCopy];
    if (data) {
        errorDict[@"data"] = data;
    }

    NSDictionary *response = @{
        @"jsonrpc": @"2.0",
        @"id": responseId,
        @"error": [errorDict copy],
    };
    context.sendText([self jsonStringFromDictionary:response]);
}

- (void)sendParseError:(NSString *)message
               context:(JigSessionContext *)context {
    [self sendErrorWithCode:JigErrorCodeParseError
                    message:message
                       data:nil
                         id:nil
                    context:context];
}

#pragma mark - JSON serialization

- (NSString *)jsonStringFromDictionary:(NSDictionary *)dict {
    NSData *data = [NSJSONSerialization dataWithJSONObject:dict
                                                  options:NSJSONWritingSortedKeys
                                                    error:nil];
    return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
}

@end
