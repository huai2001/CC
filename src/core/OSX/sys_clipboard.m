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
#include <libcc/math.h>
#include <libcc/alloc.h>

#import <Cocoa/Cocoa.h>
#include <sys/errno.h>
#include <mach-o/dyld.h>

/**/
_CC_API_PUBLIC(bool_t) _cc_set_clipboard_text(const tchar_t *str) {
    @autoreleasepool{
        NSPasteboard *pasteboard = nil;
        NSString *format = NSPasteboardTypeString;

        pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard setString:[NSString stringWithUTF8String:str] forType: format];

        return true;
    }
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_clipboard_text(tchar_t *str, int32_t len) {
    NSPasteboard *pasteboard;
    NSString *format = NSPasteboardTypeString;
    NSString *available;

    pasteboard = [NSPasteboard generalPasteboard];
    available = [pasteboard availableTypeFromArray:[NSArray arrayWithObject:format]];
    if ([available isEqualToString:format]) {
        NSString* s = [pasteboard stringForType:format];
        if (s != nil) {
            int32_t x = (int32_t)(_min(s.length, len));
            _tcsncpy(str, s.UTF8String, x);
            str[len - 1] = 0;
            return x;
        }
    }

    return 0;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_has_clipboard_text(void) {
    return false;
}

#if 0
/**/
_CC_API_PUBLIC(int32_t) _cc_get_computer_name(tchar_t *name, int32_t maxlen) {
    NSString *localizedName = [NSHost currentHost].localizedName;
    if (localizedName) {
        int32_t len = (int32_t)[localizedName lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
        len = _min(len, maxlen);
        strncpy(name, [localizedName UTF8String], len);
        name[len - 1] = 0;
        
        return len;
    }

    if (gethostname(name, maxlen) == 0) {
        return (int32_t)strlen(name);
    }
    return 0;
}
#endif