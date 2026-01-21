#include <libcc/os.h>
#include <libcc/math.h>
#include <libcc/alloc.h>

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

_CC_API_PUBLIC(size_t) _cc_get_executable_path(tchar_t *path, size_t length) {
    return 0;
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


_CC_API_PUBLIC(size_t) _cc_get_folder(_cc_folder_t folder, tchar_t *path, size_t len) {
    @autoreleasepool {
        NSString *folderPath = nil;
#ifdef __CC_APPLE_TVOS__
        _cc_logger_error("tvOS does not have persistent storage");
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
            _cc_logger_error("Invalid _cc_folder_: %d", (int) folder);
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
