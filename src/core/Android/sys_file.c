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
#include <cc/core.h>
#include <cc/core/android.h>

AAsset* Android_JNI_FileOpen(const char *fileName, const char *mode) {
    AAsset *asset = NULL;
    AAssetManager* asset_manager = _cc_jni_get_asset_manager();
    if (asset_manager == NULL) {
        return NULL;
    }
    return AAssetManager_open(asset_manager, fileName, AASSET_MODE_UNKNOWN);
}

size_t Android_JNI_FileRead(AAsset *asset, void* buffer, size_t size, size_t maxnum) {
    size_t result = AAsset_read(asset, buffer, size * maxnum);

    if (result > 0) {
        /* Number of chuncks */
        return (result / size);
    } else {
        /* Error or EOF */
        return result;
    }
}

size_t Android_JNI_FileWrite(AAsset *asset, const void *buffer, size_t size, size_t num) {
    _cc_logger_error("Cannot write to Android package filesystem");
    return 0;
}

int64_t Android_JNI_FileSize(AAsset *asset) {
    return (int64_t)AAsset_getLength64(asset);
}

int64_t Android_JNI_FileSeek(AAsset *asset, int64_t offset, int whence) {
    return (int64_t)AAsset_seek64(asset, offset, whence);;
}

void Android_JNI_FileClose(AAsset *asset) {
    AAsset_close(asset);
}