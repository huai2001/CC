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
#include <cc/loadso.h>
#include <cc/logger.h>

void *_cc_load_object(const tchar_t *sofile) {
    _cc_logger_error(_T("Failed loading %s : _cc_load_object() not implemented Failed"), sofile);
    return NULL;
}

void *_cc_load_function(void *handle, const char_t *name) {
    _cc_logger_error(_T("Failed loading : _cc_load_function() not implemented"));
    return NULL;
}

void _cc_unload_object(void *handle) {
    /* no-op. */
}