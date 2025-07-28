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

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

_CC_API_PUBLIC(size_t) _cc_get_executable_path(tchar_t *path, size_t length) {
    return _cc_get_executable_path(path, length);
}

_CC_API_PUBLIC(size_t) _cc_get_base_path(tchar_t *path, size_t length) {
    NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
    if (bundlePath) {
        size_t rc = (size_t)[bundlePath lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
        length = _min(rc, length);
        memcpy(path, [bundlePath UTF8String], length);
        path[length - 1] = 0;
    }
    return length;
}

_CC_API_PUBLIC(size_t) _cc_get_folder(_cc_folder_t folder, tchar_t *path, size_t length) {
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
            size_t rc = (size_t)[path lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
            length = _min(length, rc);
            memcpy(path, bundlePath, length);
            path[length - 1] = 0;
            return length;
        } else {
            return 0;
        }
#endif
    }
}
#if 0
/**/

/**/
_CC_API_PUBLIC(bool_t) _cc_set_current_directory(tchar_t *cwd) {
    NSString * strPath = [NSString stringWithUTF8String:(char_t*)cwd];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    [fileManager changeCurrentDirectoryPath:strPath];
    return true;
}

#endif