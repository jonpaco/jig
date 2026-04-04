#import "JigHandshakeGate.h"
#import "JigSessionContext.h"
#import "JigErrors.h"

@implementation JigHandshakeGate

- (BOOL)validateMethod:(NSString *)method
                params:(NSDictionary *)params
               context:(JigSessionContext *)context
                 error:(NSError **)error {
    if ([method isEqualToString:@"client.hello"]) {
        return YES;
    }
    if (context.sessionId == nil) {
        if (error) {
            *error = [JigErrors handshakeRequired];
        }
        return NO;
    }
    return YES;
}

@end
