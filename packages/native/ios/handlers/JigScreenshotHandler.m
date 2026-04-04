#import "JigScreenshotHandler.h"
#import "JigSessionContext.h"
#import "JigErrors.h"
#import <UIKit/UIKit.h>

@implementation JigScreenshotHandler

- (NSString *)method {
    return @"jig.screenshot";
}

- (JigThreadTarget)threadTarget {
    return JigThreadTargetMain;
}

- (id)handleParams:(NSDictionary *)params
           context:(JigSessionContext *)context
             error:(NSError **)error {

    NSString *format = params[@"format"] ?: @"png";
    NSInteger quality = params[@"quality"] ? [params[@"quality"] integerValue] : 80;

    // Find the active window
    UIWindow *window = nil;
    for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
        if ([scene isKindOfClass:[UIWindowScene class]] &&
            scene.activationState == UISceneActivationStateForegroundActive) {
            UIWindowScene *windowScene = (UIWindowScene *)scene;
            window = windowScene.windows.firstObject;
            break;
        }
    }

    if (!window) {
        if (error) {
            *error = [JigErrors internalError:@"No active window available for screenshot"];
        }
        return nil;
    }

    // Capture via CALayer.render
    UIGraphicsImageRenderer *renderer =
        [[UIGraphicsImageRenderer alloc] initWithSize:window.bounds.size];
    UIImage *image = [renderer imageWithActions:^(UIGraphicsImageRendererContext *ctx) {
        [window.layer renderInContext:ctx.CGContext];
    }];

    // Encode
    NSData *imageData;
    NSString *actualFormat;
    if ([format isEqualToString:@"jpeg"]) {
        imageData = UIImageJPEGRepresentation(image, quality / 100.0);
        actualFormat = @"jpeg";
    } else {
        imageData = UIImagePNGRepresentation(image);
        actualFormat = @"png";
    }

    NSInteger pixelWidth = (NSInteger)(image.size.width * image.scale);
    NSInteger pixelHeight = (NSInteger)(image.size.height * image.scale);
    NSInteger timestamp = (NSInteger)([[NSDate date] timeIntervalSince1970] * 1000);

    return @{
        @"data": [imageData base64EncodedStringWithOptions:0],
        @"format": actualFormat,
        @"width": @(pixelWidth),
        @"height": @(pixelHeight),
        @"timestamp": @(timestamp),
    };
}

@end
