/*
 * Copyright .Qiu<huai2011@163.com>. and other libCC contributors.
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

#include <sys/errno.h>
#include <cc/core.h>
#include <cc/math.h>
#include <cc/alloc.h>
#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

int32_t _cc_a2w(const char_t *s1, int32_t s1_len, wchar_t* s2, int32_t size) {
    return _cc_utf8_to_utf16( (const uint8_t*)s1, (const uint8_t*)(s1 + s1_len), (uint16_t*)s2, (uint16_t*)(s2 + size), false);
}

int32_t _cc_w2a(const wchar_t *s1, int32_t s1_len, char_t* s2, int32_t size) {
    return _cc_utf16_to_utf8( (const uint16_t*)s1, (const uint16_t*)(s1 + s1_len), (uint8_t*)s2, (uint8_t*)(s2 + size), false);;
}

/**/
bool_t _cc_is_simulator() {
    #if TARGET_OS_SIMULATOR
        return true;
    #elif TARGET_IPHONE_SIMULATOR
        return true;
    #else
        return false;
    #endif
}

bool_t _cc_is_system_version_at_least(double version) {
    return [[UIDevice currentDevice].systemVersion doubleValue] >= version;
}
/**/
int32_t _cc_set_clipboard_text(const tchar_t *str) {
    @autoreleasepool {
        [UIPasteboard generalPasteboard].string = @(str);
        return 0;
    }
}

/**/
int32_t _cc_get_clipboard_text(tchar_t *str, int32_t len) {
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
bool_t _cc_has_clipboard_text(void) {
    @autoreleasepool {
        if ([UIPasteboard generalPasteboard].string != nil) {
            return true;
        }
        return false;
    }
}

void _cc_set_last_errno(int32_t _errno) {
    errno = _errno;
}

int32_t _cc_last_errno(void) {
    return errno;
}

tchar_t* _cc_last_error(int32_t _errno) {
    return strerror(_errno);
}

/**/
int32_t _cc_get_computer_name(tchar_t *name, int32_t maxlen) {
    int32_t len = 0;
    NSString* phoneName = [[UIDevice currentDevice] name];
    if (phoneName == nil) {
        phoneName = [[UIDevice currentDevice] systemName];
    }

    if (phoneName) {
        len = (int32_t)[phoneName lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
        len = _min(len, maxlen);
        _tcsncpy(name, [phoneName UTF8String], len);
        name[len - 1] = 0;
    }
    return len;
}

/**/
int32_t _cc_get_current_directory(tchar_t *cwd, int32_t maxlen) {
    getcwd(cwd, maxlen);
    return (int32_t)strlen(cwd);
}

/**/
int32_t _cc_get_current_file(tchar_t *cwd, int32_t maxlen) {
    NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
    int32_t len = (int32_t)[bundlePath lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
    len = _min(len, maxlen);

    strncpy(cwd, [bundlePath UTF8String], len);
    cwd[len - 1] = 0;

    return len;
}

/**/
int32_t _cc_get_module_document_directory(tchar_t *cwd, int32_t maxlen) {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *path = [paths objectAtIndex:0];
    int32_t len = (int32_t)[path lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
    len = _min(len, maxlen);
    strncpy(cwd, [path UTF8String], len);
    cwd[len - 1] = 0;
    return len;

}
/**/
int32_t _cc_get_module_cache_directory(tchar_t *cwd, int32_t maxlen) {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    NSString *path = [paths objectAtIndex:0];
    int32_t len = (int32_t)[path lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
    len = _min(len, maxlen);
    strncpy(cwd, [path UTF8String], len);
    cwd[len - 1] = 0;
    return len;
}

/**/
int32_t _cc_get_module_file_name(tchar_t *cwd, int32_t maxlen) {
    const tchar_t* proc_name;
    const tchar_t* proc_path;
    int32_t len = 0;

    NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
    proc_path = [bundlePath UTF8String];
    proc_name = strrchr(proc_path, _CC_PATH_SEP_C_);
    if (proc_name) {
        proc_name++;
        while(*proc_name) {
            cwd[len++] = *(proc_name++);
        }
    }
    cwd[len] = 0;
    
    return len;
}

/**/
int32_t _cc_get_module_directory(const tchar_t *module, tchar_t *cwd, int32_t maxlen) {
    int32_t len = 0;
    if (module) {
        NSString * bundlePath = [[ NSBundle mainBundle] pathForResource: [NSString stringWithUTF8String:module] ofType :@ ""];
        if (bundlePath) {
            len = (int32_t)[bundlePath lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
            len = _min(len, maxlen);
            _tcsncpy(cwd, [bundlePath UTF8String], len);
            cwd[len - 1] = 0;

            return len;
        }
    }
    NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
    if (module != NULL) {
        return _sntprintf(cwd, maxlen, "%s/%s", [bundlePath UTF8String], module);
    }
    
    _tcsncpy(cwd, [bundlePath UTF8String], maxlen);
    cwd[maxlen - 1] = 0;
    return maxlen;
}

/**/
bool_t _cc_set_current_directory(tchar_t *cwd) {
    NSString * strPath = [NSString stringWithUTF8String:(char_t*)cwd];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    [fileManager changeCurrentDirectoryPath:strPath];
    return true;
}
