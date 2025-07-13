#import <UIKit/UIKit.h>
#include <libcc/core.h>

_CC_API_PUBLIC(bool_t) _cc_open_url(const tchar_t *url) {
    @autoreleasepool {
        NSString *nsstr = [NSString stringWithUTF8String:url];
        NSURL *nsurl = [NSURL URLWithString:nsstr];
        if (![[UIApplication sharedApplication] canOpenURL:nsurl]) {
            return false;
        }
        [[UIApplication sharedApplication] openURL:nsurl options:@{} completionHandler:^(BOOL success) {}];
        return true;
    }
}