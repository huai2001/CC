/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
 * All rights reserved.org>
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:

 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/

#include <libcc/generic.h>
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
