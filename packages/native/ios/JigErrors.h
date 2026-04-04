#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

extern NSString *const JigErrorDomain;

/** Key in NSError.userInfo for structured error data (NSDictionary) */
extern NSString *const JigErrorDataKey;

typedef NS_ENUM(NSInteger, JigErrorCode) {
    JigErrorCodeParseError          = -32700,
    JigErrorCodeInvalidRequest      = -32600,
    JigErrorCodeMethodNotFound      = -32601,
    JigErrorCodeInvalidParams       = -32602,
    JigErrorCodeInternalError       = -32603,
    JigErrorCodeHandshakeRequired   = -32000,
    JigErrorCodeProtocolVersion     = -32001,
    JigErrorCodeElementNotFound     = -32002,
    JigErrorCodeTimeout             = -32003,
    JigErrorCodeInvalidSelector     = -32004,
    JigErrorCodeUnsupportedDomain   = -32005,
    JigErrorCodeDomainNotEnabled    = -32006,
};

@interface JigErrors : NSObject

+ (NSError *)parseError:(NSString *)message;
+ (NSError *)invalidRequest:(NSString *)message;
+ (NSError *)methodNotFound:(NSString *)method;
+ (NSError *)invalidParams:(NSString *)message;
+ (NSError *)internalError:(NSString *)message;
+ (NSError *)handshakeRequired;
+ (NSError *)protocolVersionError:(NSInteger)requested
                        supported:(NSDictionary *)supported;
+ (NSError *)elementNotFound:(NSString *)message data:(nullable NSDictionary *)data;
+ (NSError *)timeout:(NSString *)message;
+ (NSError *)invalidSelector:(NSString *)message;
+ (NSError *)unsupportedDomain:(NSString *)domain;
+ (NSError *)domainNotEnabled:(NSString *)domain;

@end

NS_ASSUME_NONNULL_END
