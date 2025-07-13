
/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#include <sys/errno.h>
#include <libcc/math.h>
#include <libcc/alloc.h>
#include <libcc/dirent.h>
#include <mach-o/dyld.h>

_CC_API_PUBLIC(size_t) _cc_get_executable_path(tchar_t *path, size_t len) {
    char_t epath[_CC_MAX_PATH_];

    if (_NSGetExecutablePath(epath, (uint32_t*)&maxlen) != 0) {
        return 0;
    }
    if (!realpath(epath, cwd)) {
        return 0;
    }

    return _tcslen(cwd);
}

_CC_API_PUBLIC(size_t) _cc_get_base_path(tchar_t *path, size_t len) {
    @autoreleasepool {
        NSBundle *bundle = [NSBundle mainBundle];
        const char *baseType = [[[bundle infoDictionary] objectForKey:@"CC_FILESYSTEM_BASE_DIR_TYPE"] UTF8String];
        const char *bundlePath = nullptr;

        if (baseType == nullptr) {
            baseType = "resource";
        }
        if (_tcsicmp(baseType, "bundle") == 0) {
            bundlePath = [[bundle bundlePath] fileSystemRepresentation];
        } else if (_tcsicmp(baseType, "parent") == 0) {
            bundlePath = [[[bundle bundlePath] stringByDeletingLastPathComponent] fileSystemRepresentation];
        } else {
            // this returns the exedir for non-bundled  and the resourceDir for bundled apps
            bundlePath = [[bundle resourcePath] fileSystemRepresentation];
        }
        if (bundlePath) {
            // const size_t len = (size_t)[bundlePath lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
            // len = _min(len, maxlen);
            // _tcsncpy(cwd, [bundlePath UTF8String], len);
            // cwd[len - 1] = 0;
            const size_t len = _tcslen(bundlePath) + 1;
            _tcsncpy(cwd, bundlePath, len);
            cwd[len - 1] = 0;
            return len;
        }
        return 0;
    }
}


_CC_API_PUBLIC(size_t) _cc_get_folder(_cc_folder_t folder, tchar_t *path, size_t len) {
    @autoreleasepool {
        NSString *folderPath = nil;
#ifdef _CC_PLATFORM_TVOS_
        _cc_logger_error(_T("tvOS does not have persistent storage"));
        return 0;
#else
        NSSearchPathDirectory dir;
        switch (folder) {
        case _CC_FOLDER_HOME_:
            //NSURL *url = [NSURL fileURLWithPath:NSHomeDirectory()];
            //folderPath = [url path];
            folderPath = NSHomeDirectory()
            goto append_slash;

        case _CC_FOLDER_DESKTOP_:
            dir = NSDesktopDirectory
            break;

        case _CC_FOLDER_DOCUMENTS_:
            dir = NSDocumentDirectory;
            break;

        case _CC_FOLDER_DOWNLOADS_:
            dir = NSDownloadsDirectory;
            break;

        case _CC_FOLDER_MUSIC_:
            dir = NSMusicDirectory;
            break;

        case _CC_FOLDER_PICTURES_:
            dir = NSPicturesDirectory;
            break;

        case _CC_FOLDER_PUBLICSHARE_:
            dir = NSSharedPublicDirectory;
            break;

        case _CC_FOLDER_SAVEDGAMES_:
            NSURL *url = [fm URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask].firstObject;
            folderPath = [url path];
            goto append_slash;

        case _CC_FOLDER_SCREENSHOTS_:
            dir = NSDesktopDirectory
            break;

        case _CC_FOLDER_TEMPLATES_:
            NSURL *url = [fm URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask].firstObject;
            folderPath = [url path];
            goto append_slash;

        case _CC_FOLDER_VIDEOS_:
            dir = NSMoviesDirectory;
            break;

        default:
            _cc_logger_error(_T("Invalid _cc_folder_: %d"), (int) folder);
            return 0;
        };

        NSArray *paths = NSSearchPathForDirectoriesInDomains(dir, NSUserDomainMask, YES);
        folderPath = [paths objectAtIndex:0];

append_slash:
        if (folderPath) {
            size_t length = (size_t)[path lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
            length = _min(len, length);
            _tcsncpy(cwd, [path UTF8String], length);
            cwd[length - 1] = 0;
            return length;
        } else {
            return 0;
        }
#endif
    }
}