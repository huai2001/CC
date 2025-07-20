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
#include <libcc/loadso.h>
#include <libcc/utils.h>
#include <stdio.h>
// #include <be/kernel/image.h>
#include <kernel/image.h>

_CC_API_PUBLIC(void*) _cc_load_object(const tchar_t *sofile) {
    image_id library_id = load_add_on(sofile);
    if (library_id < 0) {
        _cc_logger_error(_T("Failed load_add_on %s : %s"), sofile, strerror((int)library_id));
        return nullptr;
    }

    return (void *)(library_id);
}

_CC_API_PUBLIC(void*) _cc_load_function(void *handle, const char_t *name) {
    void *sym = nullptr;
    image_id library_id = (image_id)handle;
    status_t rc = get_image_symbol(library_id, name, B_SYMBOL_TYPE_TEXT, &sym);
    if (rc != B_NO_ERROR) {
        _cc_logger_error(_T("Failed get_image_symbol %s : %s"), name, strerror((int)rc));
        return nullptr;
    }
    return (sym);
}

_CC_API_PUBLIC(void) _cc_unload_object(void *handle) {
    if (handle != nullptr) {
        unload_add_on((image_id)handle);
    }
}