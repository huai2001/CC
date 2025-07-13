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
#include <libcc/alloc.h>
#import <Foundation/Foundation.h>
/*
_CC_API_PUBLIC(const byte_t*) _cc_ios_file_context(const char_t *file,
                                    const char_t *type,
                                    int32_t *file_len) {
    NSString *fileName;
    NSString *fileType;
    NSString *path_name;
    byte_t *source = nullptr;
    
    fileName = [NSString stringWithUTF8String:file];
    fileType = [NSString stringWithUTF8String:type];
    
    path_name = [[NSBundle mainBundle] pathForResource:fileName ofType:fileType];
    source = (byte_t *)[[NSString stringWithContentsOfFile:path_name encoding:NSUTF8StringEncoding error:nil] UTF8String];
    if (!source) {
        NSLog(@"Failed to load file:%s.%s", file, type);
        return nullptr;
    }
    
    return source;
}
*/
_CC_API_PUBLIC(bool_t) _cc_file_is_exist_of_path(const tchar_t *filename) {
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *filePath = [NSString stringWithUTF8String:filename];
    
    return [fileManager fileExistsAtPath:filePath] ? true : false;
}

_CC_API_PUBLIC(int) _cc_ios_unlink(const tchar_t *filename) {
    NSFileManager *fileManage = [NSFileManager defaultManager];
    NSString *filePath = [NSString stringWithUTF8String:filename];
    
    if ([fileManage fileExistsAtPath:filePath]) {
        return [fileManage removeItemAtPath:filePath error:nil] ? 0 : 1;
    }
    return -1;
}
/*
_CC_API_PRIVATE(BOOL) _cc_create_file_with_path(NSString *filePath) {
    BOOL isSuccess = YES;
    NSFileManager *fileManager = [NSFileManager defaultManager];
    if ([fileManager fileExistsAtPath:filePath]) {
        return YES;
    }

    NSError *error;
    NSString *dirPath = [filePath stringByDeletingLastPathComponent];
    isSuccess = [fileManager createDirectoryAtPath:dirPath withIntermediateDirectories:YES attributes:nil error:&error];
    if (error) {
        NSLog(@"creat File Failed. errorInfo:%@",error);
    }

    if (!isSuccess) {
        return isSuccess;
    }

    isSuccess = [fileManager createFileAtPath:filePath contents:nil attributes:nil];
    return isSuccess;
}
*/

