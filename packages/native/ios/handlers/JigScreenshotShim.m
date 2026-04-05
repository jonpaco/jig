// ios/handlers/JigScreenshotShim.m
//
// Screenshot handler shim — wraps UIKit screenshot capture as a C core jig_handler.

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

#include "../../core/jig_handler.h"
#include "../../core/jig_errors.h"
#include "../../core/vendor/cJSON/cJSON.h"

static cJSON *screenshot_handle(jig_handler *self, cJSON *params,
                                jig_session *session, jig_error **err) {
    /* Parse params */
    const char *format_str = "png";
    int quality = 80;

    if (params) {
        cJSON *fmt = cJSON_GetObjectItemCaseSensitive(params, "format");
        if (cJSON_IsString(fmt) && fmt->valuestring) {
            format_str = fmt->valuestring;
        }
        cJSON *q = cJSON_GetObjectItemCaseSensitive(params, "quality");
        if (cJSON_IsNumber(q)) {
            quality = q->valueint;
        }
    }

    /* Find the active window */
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
        *err = jig_error_internal("No active window available for screenshot");
        return NULL;
    }

    /* Capture via CALayer.render */
    UIGraphicsImageRenderer *renderer =
        [[UIGraphicsImageRenderer alloc] initWithSize:window.bounds.size];
    UIImage *image = [renderer imageWithActions:^(UIGraphicsImageRendererContext *ctx) {
        [window.layer renderInContext:ctx.CGContext];
    }];

    /* Encode */
    NSData *imageData;
    const char *actualFormat;
    if (strcmp(format_str, "jpeg") == 0) {
        imageData = UIImageJPEGRepresentation(image, quality / 100.0);
        actualFormat = "jpeg";
    } else {
        imageData = UIImagePNGRepresentation(image);
        actualFormat = "png";
    }

    NSInteger pixelWidth = (NSInteger)(image.size.width * image.scale);
    NSInteger pixelHeight = (NSInteger)(image.size.height * image.scale);
    NSInteger timestamp = (NSInteger)([[NSDate date] timeIntervalSince1970] * 1000);

    NSString *base64 = [imageData base64EncodedStringWithOptions:0];

    /* Build cJSON result */
    cJSON *result = cJSON_CreateObject();
    cJSON_AddStringToObject(result, "data", [base64 UTF8String]);
    cJSON_AddStringToObject(result, "format", actualFormat);
    cJSON_AddNumberToObject(result, "width", (double)pixelWidth);
    cJSON_AddNumberToObject(result, "height", (double)pixelHeight);
    cJSON_AddNumberToObject(result, "timestamp", (double)timestamp);

    return result;
}

jig_handler *jig_screenshot_handler_create(void) {
    jig_handler *h = calloc(1, sizeof(jig_handler));
    h->method = "jig.screenshot";
    h->thread_target = JIG_THREAD_MAIN;
    h->handle = screenshot_handle;
    return h;
}
