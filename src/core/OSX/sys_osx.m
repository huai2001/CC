#import <Cocoa/Cocoa.h>
#include <libcc/logger.h>

_CC_API_PUBLIC(bool_t) _cc_open_url(const tchar_t *url) {
    @autoreleasepool {
        CFURLRef cfurl = CFURLCreateWithBytes(NULL, (const UInt8 *)url, SDL_strlen(url), kCFStringEncodingUTF8, NULL);
        OSStatus status = LSOpenCFURLRef(cfurl, NULL);
        CFRelease(cfurl);
        if (status != noErr) {
            _cc_logger_error(_T("LSOpenCFURLRef() failed: %d"), status);
            return false;
        }
        return true;
    }
}