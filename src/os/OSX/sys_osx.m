#import <Cocoa/Cocoa.h>
#import <sys/sysctl.h>
#include <libcc/logger.h>

_CC_API_PUBLIC(bool_t) _cc_open_url(const tchar_t *url) {
    @autoreleasepool {
        CFURLRef cfurl = CFURLCreateWithBytes(NULL, (const UInt8 *)url, _tcslen(url), kCFStringEncodingUTF8, NULL);
        OSStatus status = LSOpenCFURLRef(cfurl, NULL);
        CFRelease(cfurl);
        if (status != noErr) {
            _cc_logger_error(_T("LSOpenCFURLRef() failed: %d"), status);
            return false;
        }
        return true;
    }
}

_CC_API_PUBLIC(size_t) _cc_get_device_name(tchar_t *cname, size_t length) {
    sysctlbyname("kern.hostname", cname, &length, NULL, 0);
    return length;
}
