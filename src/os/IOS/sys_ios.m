#import <UIKit/UIKit.h>
#include <libcc/os.h>

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

_CC_API_PUBLIC(size_t) _cc_get_device_name(tchar_t *cname, size_t length) {
    NSString *userPhoneName = [[UIDevice currentDevice] name];
    length = length > userPhoneName.length ? (userPhoneName.length + 1) : length;
    memcpy(cname, [userPhoneName UTF8String], length * sizeof(tchar_t));
    cname[length - 1] = 0;
    return length;
}
