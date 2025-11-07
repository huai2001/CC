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