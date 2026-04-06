#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#include "JigViewWalker.h"
#include "../../core/vendor/cJSON/cJSON.h"

static const char *role_from_traits(UIAccessibilityTraits traits) {
    if (traits & UIAccessibilityTraitButton) return "button";
    if (traits & UIAccessibilityTraitLink) return "link";
    if (traits & UIAccessibilityTraitImage) return "image";
    if (traits & UIAccessibilityTraitSearchField) return "search";
    if (traits & UIAccessibilityTraitHeader) return "header";
    if (traits & UIAccessibilityTraitAdjustable) return "adjustable";
    if (traits & UIAccessibilityTraitStaticText) return "text";
    return NULL;
}

static NSString *text_from_view(UIView *view) {
    if ([view isKindOfClass:[UILabel class]]) {
        return ((UILabel *)view).text;
    }
    if ([view isKindOfClass:[UITextField class]]) {
        UITextField *tf = (UITextField *)view;
        return tf.text.length > 0 ? tf.text : tf.placeholder;
    }
    if ([view isKindOfClass:[UITextView class]]) {
        return ((UITextView *)view).text;
    }
    return view.accessibilityLabel;
}

static BOOL is_visible(UIView *view, UIWindow *window) {
    if (view.isHidden || view.alpha <= 0) return NO;
    CGRect screenFrame = [view convertRect:view.bounds toView:nil];
    if (CGRectIsEmpty(screenFrame)) return NO;
    return CGRectIntersectsRect(screenFrame, window.bounds);
}

static void walk_view(UIView *view, int parentTag, UIWindow *window, cJSON *results) {
    NSInteger reactTag = view.tag;
    BOOL hasTestID = (view.accessibilityIdentifier.length > 0);
    BOOL hasLabel = (view.accessibilityLabel.length > 0);
    if (reactTag <= 0 && !hasTestID && !hasLabel) {
        for (UIView *child in view.subviews) {
            walk_view(child, parentTag, window, results);
        }
        return;
    }

    int tag = (int)reactTag;
    cJSON *el = cJSON_CreateObject();
    cJSON_AddNumberToObject(el, "reactTag", tag);
    if (parentTag >= 0) {
        cJSON_AddNumberToObject(el, "parentReactTag", parentTag);
    }

    if (view.accessibilityIdentifier.length > 0) {
        cJSON_AddStringToObject(el, "testID",
            [view.accessibilityIdentifier UTF8String]);
    }

    NSString *text = text_from_view(view);
    if (text.length > 0) {
        cJSON_AddStringToObject(el, "text", [text UTF8String]);
    }

    const char *role = role_from_traits(view.accessibilityTraits);
    if (role) {
        cJSON_AddStringToObject(el, "role", role);
    }

    CGRect screenFrame = [view convertRect:view.bounds toView:nil];
    cJSON *frame = cJSON_CreateObject();
    cJSON_AddNumberToObject(frame, "x", screenFrame.origin.x);
    cJSON_AddNumberToObject(frame, "y", screenFrame.origin.y);
    cJSON_AddNumberToObject(frame, "width", screenFrame.size.width);
    cJSON_AddNumberToObject(frame, "height", screenFrame.size.height);
    cJSON_AddItemToObject(el, "frame", frame);

    cJSON_AddBoolToObject(el, "visible", is_visible(view, window));

    cJSON_AddItemToArray(results, el);

    for (UIView *child in view.subviews) {
        walk_view(child, tag, window, results);
    }
}

cJSON *jig_ios_walk_views(void) {
    cJSON *results = cJSON_CreateArray();

    UIWindow *window = nil;
    for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
        if ([scene isKindOfClass:[UIWindowScene class]] &&
            scene.activationState == UISceneActivationStateForegroundActive) {
            UIWindowScene *windowScene = (UIWindowScene *)scene;
            window = windowScene.windows.firstObject;
            break;
        }
    }

    if (!window) return results;
    walk_view(window, -1, window, results);
    return results;
}
