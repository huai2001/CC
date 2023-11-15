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
#import <Cocoa/Cocoa.h>
#include <sys/errno.h>
#include <cc/math.h>
#include <cc/alloc.h>
#include <cc/dirent.h>
#include <mach-o/dyld.h>

_CC_API_PUBLIC(int32_t) _cc_a2w(const char_t *s1, int32_t s1_len, wchar_t* s2, int32_t size) {
    return _cc_utf8_to_utf16( (const uint8_t*)s1, (const uint8_t*)(s1 + s1_len), (uint16_t*)s2, (uint16_t*)(s2 + size), false);
}

_CC_API_PUBLIC(int32_t) _cc_w2a(const wchar_t *s1, int32_t s1_len, char_t* s2, int32_t size) {
    return _cc_utf16_to_utf8( (const uint16_t*)s1, (const uint16_t*)(s1 + s1_len), (uint8_t*)s2, (uint8_t*)(s2 + size), false);;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_set_clipboard_text(const tchar_t *str) {
    @autoreleasepool{
        NSPasteboard *pasteboard = nil;
        NSString *format = NSPasteboardTypeString;

        pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard setString:[NSString stringWithUTF8String:str] forType: format];

        return 0;
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

_CC_API_PUBLIC(void) _cc_set_last_errno(int32_t _errno) {
    errno = _errno;
}

_CC_API_PUBLIC(int32_t) _cc_last_errno(void) {
    return errno;
}

_CC_API_PUBLIC(tchar_t*) _cc_last_error(int32_t _errno) {
    return strerror(_errno);
}

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

/**/
_CC_API_PUBLIC(int32_t) _cc_get_current_directory(tchar_t *cwd, int32_t maxlen) {
    if (getcwd(cwd, maxlen) != NULL) {
        return (int32_t)strlen(cwd);
    }
    return 0;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_current_file(tchar_t *cwd, int32_t maxlen) {
    uint32_t rlen = _CC_MAX_PATH_;
    char_t proc_path[_CC_MAX_PATH_];
    if (_NSGetExecutablePath(proc_path, &rlen) != 0) {
        return 0;
    }
    
    if (!realpath(proc_path, cwd)) {
        return 0;
    }

    
    return (int32_t)strlen(cwd);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_module_file_name(tchar_t *cwd, int32_t maxlen) {
    char_t proc_path[_CC_MAX_PATH_];
    char_t* proc_name = NULL;
    uint32_t size = _CC_MAX_PATH_;
    int32_t len = 0;
    
    if (maxlen <= 0) {
        return 0;
    }
    
    if (_NSGetExecutablePath(proc_path, &size) != 0) {
        return 0;
    }
    
    if (!realpath(proc_path, cwd)) {
        return 0;
    }

    proc_name = strrchr(cwd, _CC_PATH_SEP_C_);
    if (proc_name) {
        proc_name++;
        while(*proc_name && len < maxlen) {
            cwd[len++] = *(proc_name++);
        }
    }
    cwd[len] = 0;
    return len;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_module_document_directory(tchar_t *cwd, int32_t maxlen) {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *docuemntPath = [paths objectAtIndex:0];
    int32_t len = (int32_t)[docuemntPath lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
    len = _min(len, maxlen);
    _tcsncpy(cwd, [docuemntPath UTF8String], len);
    cwd[len - 1] = 0;
    
    return len;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_module_cache_directory(tchar_t *cwd, int32_t maxlen) {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    NSString *cachePath = [paths objectAtIndex:0];
    int32_t len = (int32_t)[cachePath lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
    len = _min(len, maxlen);
    _tcsncpy(cwd, [cachePath UTF8String], len);
    cwd[len - 1] = 0;
    
    return (int32_t)len;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_executable_directory(tchar_t *cwd, int32_t maxlen) {
    char_t epath[_CC_MAX_PATH_];

    if (_NSGetExecutablePath(epath, (uint32_t*)&maxlen) != 0) {
        return 0;
    }
    if (!realpath(epath, cwd)) {
        return 0;
    }

    return (int32_t)strlen(cwd);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_module_directory(const tchar_t *module, tchar_t *cwd, int32_t maxlen) {
    uint32_t size = maxlen;
    NSString *bundlePath = NULL;

    if (module) {
        bundlePath = [[NSBundle mainBundle] pathForResource: [NSString stringWithUTF8String:module] ofType :@ ""];
    } else {
        bundlePath = [[NSBundle mainBundle] bundlePath];
    }
    
    if (bundlePath) {
        size = (int32_t)[bundlePath lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
        size = _min(size, maxlen);
        _tcsncpy(cwd, [bundlePath UTF8String], size);
        cwd[size - 1] = 0;
        return size;
    } else  {
        int32_t i = 0;
        int32_t rc = 0;
        tchar_t epath[_CC_MAX_PATH_];

        if (_NSGetExecutablePath(epath, &size) != 0) {
            return 0;
        }
        
        if (!realpath(epath, cwd)) {
            return 0;
        }

        rc = (int32_t)strlen(cwd);
        for (i = rc - 1; i >= 0; i--) {
            if (cwd[i] == _CC_T_PATH_SEP_C_) {
                /* chop off filename. */
                cwd[i++] = 0;
                break;
            }
        }

        if (module && i < maxlen) {
            char_t *p = (char_t*)module;
            cwd[i - 1] = _CC_T_PATH_SEP_C_;
            while(i < maxlen) {
                cwd[i++] = *p++;
                if (*p == 0) {
                    break;
                }
            }
            cwd[i++] = 0;
        }
        return i;
    }

}

/**/
_CC_API_PUBLIC(bool_t) _cc_set_current_directory(tchar_t *cwd) {
    NSString * strPath = [NSString stringWithUTF8String:(char_t*)cwd];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    [fileManager changeCurrentDirectoryPath:strPath];
    return true;
}
