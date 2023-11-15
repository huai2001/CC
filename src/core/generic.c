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
#include <cc/alloc.h>
#include <cc/dirent.h>

/**/
_CC_API_PUBLIC(bool_t) _cc_mkdir(const tchar_t *path) {
    _CC_API_PUBLIC(return) _cc_create_directory(path, false);
}

/**/
_CC_API_PUBLIC(bool_t) _cc_create_directory(const tchar_t *path, bool_t is_dir) {
    int32_t i = 0;
    const tchar_t *cp = NULL;
    tchar_t cpath[_CC_MAX_PATH_];

    cp = path;
    
    /* Skip the first / */
    if (_CC_IS_PATH_SEPARATOR(*cp)) {
        cpath[i++] = *cp++;
    }

    /**/
    while (*cp) {
        if (_CC_IS_PATH_SEPARATOR(*cp)) {
            cpath[i] = 0;
            if (_taccess(cpath, _CC_ACCESS_F_) == -1) {
                _tmkdir(cpath);
            }
        }
        cpath[i++] = *cp++;
    }

    if (is_dir) {
        cpath[i] = 0;
        if (_taccess(cpath, 0) != 0) {
            _tmkdir(cpath);
        }
    }
    return true;
}

_CC_API_PUBLIC(uint32_t) _cc_nextpow2(uint32_t num) {
    --num;
    num |= num >> 1;
    num |= num >> 2;
    num |= num >> 4;
    num |= num >> 8;
    num |= num >> 16;

    return ++num;
}

_CC_API_PUBLIC(int32_t) _cc_realpath(tchar_t *path) {
    int32_t i;
    int32_t k;
    int32_t rc = 1;
    size_t j;

    _cc_str_t split[_CC_MAX_PATH_ / 2];
    _cc_str_t out[_CC_MAX_PATH_ / 2];
    _cc_str_t *d = &split[0];
    tchar_t *p = path;

    d->data = p;
    while (*p) {
        if (*p == _T('/') || *p == _T('\\')) {
            d->length = (size_t)(p - d->data);
            d = &split[rc++];
            d->data = p + 1;
        }
        p++;
    }

    if (rc == 0) {
        return (int32_t)(p - d->data);
    }

    d->length = (size_t)(p - d->data);

    d = &split[0];
    k = 0;

    p = path;

    if (d->length == 0) {
        i = 1;
        *p++ = _CC_T_PATH_SEP_C_;
    } else {
        i = 0;
    }

    for (; i < rc; i++) {
        d = &split[i];

        if (d->length == 0) {
            continue;
        }
        if (*d->data == '.') {
            if (d->length == 2 && *(d->data + 1) == '.') {
                if (k > 0) {
                    k--;
                }
                continue;
            }
            if (d->length == 1) {
                continue;
            }
        }
        out[k++] = split[i];
    }

    for (i = 0; i < k; i++) {
        d = &out[i];
        for (j = 0; j < d->length; j++) {
            *p++ = *(d->data + j);
        }
        *p++ = _CC_PATH_SEP_C_;
    }
    *(p - 1) = 0;
    return (int32_t)(p - path);
}
