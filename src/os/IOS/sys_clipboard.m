#include <libcc/platform.h>
#include <libcc/math.h>
#include <libcc/alloc.h>

#include <sys/errno.h>
#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

/**/
_CC_API_PUBLIC(bool_t) _cc_set_clipboard_text(const tchar_t *str) {
    @autoreleasepool {
        [UIPasteboard generalPasteboard].string = @(str);
        return true;
    }
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_clipboard_text(tchar_t *str, int32_t len) {
    @autoreleasepool {
        UIPasteboard *p = [UIPasteboard generalPasteboard];
        NSString * s = p.string;
        if (s != nil) {
            int32_t x = _min((int32_t)(s.length), len);
            _tcsncpy(str, s.UTF8String, x);
            str[len - 1] = 0;
            return x;
        } else {
            return 0;
        }
    }
}

/**/
_CC_API_PUBLIC(bool_t) _cc_has_clipboard_text(void) {
    @autoreleasepool {
        if ([UIPasteboard generalPasteboard].string != nil) {
            return true;
        }
        return false;
    }
}


/**/
_CC_API_PUBLIC(bool_t) _cc_is_simulator(void) {
    #if TARGET_OS_SIMULATOR
        return true;
    #elif TARGET_IPHONE_SIMULATOR
        return true;
    #else
        return false;
    #endif
}

_CC_API_PUBLIC(bool_t) _cc_is_system_version_at_least(double version) {
    return [[UIDevice currentDevice].systemVersion doubleValue] >= version;
}
