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
#include <libcc/alloc.h>
#import <Foundation/Foundation.h>

_CC_API_PRIVATE(FILE*) _osx_open_file_m(const tchar_t *file, const tchar_t *mode) {
    @autoreleasepool {
        FILE* fp = nullptr;

        /* If the file mode is read, skip all the bundle stuff because generally the bundle is read-only. */
        if (*mode != 'r' && *(mode + 1) != 'b') {
            return _tfopen(file, mode);
        }

        NSFileManager* file_manager = [NSFileManager defaultManager];
        NSString* resource_path = [[NSBundle mainBundle] resourcePath];

        NSString* ns_string_file_component = [file_manager stringWithFileSystemRepresentation:file length:strlen(file)];

        NSString* full_path_with_file_to_try = [resource_path stringByAppendingPathComponent:ns_string_file_component];
        if ([file_manager fileExistsAtPath:full_path_with_file_to_try]) {
            fp = fopen([full_path_with_file_to_try fileSystemRepresentation], mode);
        } else {
            fp = fopen(file, mode);
        }
        return fp;
    }
}

/**/
_CC_API_PUBLIC(bool_t) _cc_sys_open_file(_cc_file_t *f, const tchar_t *filename, const tchar_t *mode) {
    f->fp = (pvoid_t)_osx_open_file_m(filename, mode);
    return f->fp != nullptr;
}
