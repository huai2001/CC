

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#include <sys/errno.h>
#include <libcc/math.h>
#include <libcc/alloc.h>
#include <libcc/dirent.h>
#include <mach-o/dyld.h>

_CC_API_PUBLIC(size_t) _cc_get_executable_path(tchar_t *path, size_t length) {
    tchar_t cwd[_CC_MAX_PATH_];
    uint32_t cwd_length = _cc_countof(cwd);
    _CC_UNUSED(length);

    if (_NSGetExecutablePath(cwd, (uint32_t*)&cwd_length) != 0) {
        return 0;
    }

    if (!realpath(cwd, path)) {
        return 0;
    }

    return _tcslen(path);
}

_CC_API_PUBLIC(size_t) _cc_get_base_path(tchar_t *path, size_t length) {
    @autoreleasepool {
        NSBundle *bundle = [NSBundle mainBundle];
        const char *baseType = [[[bundle infoDictionary] objectForKey:@"CC_FILESYSTEM_BASE_DIR_TYPE"] UTF8String];
        const char *bundlePath = NULL;

        if (baseType == NULL) {
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
            const size_t rc = _tcslen(bundlePath) + 1;
            length = _min(length, rc);
            memcpy(path, bundlePath, length);
            path[length - 1] = 0;
            return length;
        }

        return 0;
    }
}


_CC_API_PUBLIC(size_t) _cc_get_folder(_cc_folder_t folder, tchar_t *path, size_t len) {
    @autoreleasepool {
        NSString *folderPath = nil;
#ifdef __CC_APPLE_TVOS__
        _cc_logger_error(_T("tvOS does not have persistent storage"));
        return 0;
#else
        NSSearchPathDirectory dir;
        switch (folder) {
        case _CC_FOLDER_HOME_:
            dir = NSUserDirectory;
            break;
        case _CC_FOLDER_DESKTOP_:
            dir = NSDesktopDirectory;
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
            dir = NSDocumentDirectory;
            break;
        case _CC_FOLDER_SCREENSHOTS_:
            dir = NSDesktopDirectory;
            break;
        case _CC_FOLDER_TEMPLATES_:
            //NSTemporaryDirectory();
            dir = NSApplicationSupportDirectory;
            break;
        case _CC_FOLDER_VIDEOS_:
            dir = NSMoviesDirectory;
            break;
        default:
            _cc_logger_error(_T("Invalid _cc_folder_: %d"), (int) folder);
            return 0;
        };

        NSArray *paths = NSSearchPathForDirectoriesInDomains(dir, NSUserDomainMask, YES);
        folderPath = [paths objectAtIndex:0];

        if (folderPath) {
            size_t length = (size_t)[folderPath lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
            length = _min(len, length);
            _tcsncpy(path, [folderPath UTF8String], length);
            path[length - 1] = 0;
            return length;
        } else {
            return 0;
        }
#endif
    }
}