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

_CC_API_PUBLIC(size_t) _cc_get_executable_path(tchar_t *path, size_t length) {
    _cc_logger_error(_T("That operation is not supported"));
    return 0;
}

_CC_API_PUBLIC(size_t) _cc_get_base_path(tchar_t *path, size_t length) {
    const _cc_String_t * fpath = GetAndroidExternalStoragePath();
    length = _min(fpath.length, length);
    memcpy(path, fpath.data, length);
    path[length - 1] = 0;
    return length;
}

_CC_API_PUBLIC(size_t) _cc_get_folder(_cc_folder_t folder, tchar_t *path, size_t length) {
    const _cc_String_t * fpath;
    if (folder == _CC_FOLDER_TEMPLATES_) {
        fpath = GetAndroidCachePath();
    } else {
        fpath = GetAndroidExternalStoragePath();
    }

    length = _min(fpath.length, length);
    memcpy(path, fpath.data, length);
    path[length - 1] = 0;
    return length;
}