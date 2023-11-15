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
#ifndef _C_CC_ANDROID_H_INCLUDED_
#define _C_CC_ANDROID_H_INCLUDED_

#include "../types.h"

#ifdef __CC_ANDROID__

/*******************************************************************************
 This file links the Java side of Android with libcc
*******************************************************************************/
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <jni.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef _access
#define _access access
#endif
/**
 * @brief JNI Console log
 */
#define _CC_ANDROID_TAG_ "CC-JNI"

JNIEnv *_cc_jni_get_env(void);

int _cc_jni_setup_thread(void);

AAsset* Android_JNI_FileOpen(const char *fileName, const char *mode);
size_t Android_JNI_FileRead(AAsset *asset, void* buffer, size_t size, size_t maxnum);
size_t Android_JNI_FileWrite(AAsset *asset, const void *buffer, size_t size, size_t num);
int64_t Android_JNI_FileSize(AAsset *asset);
int64_t Android_JNI_FileSeek(AAsset *asset, int64_t offset, int whence);
void Android_JNI_FileClose(AAsset *asset);

_CC_API_PUBLIC(bool_t) _cc_jni_get_locale(char *buf, size_t buflen);

/**
 * @brief JNI onload(JNI_OnLoad) Android Virtual Machine System.loadLibrary()
 *
 * @param vm JavaVM
 * @param reserved reserved
 * @param version JNI Version
 *
 * @return JNIEnv
 */
_CC_API_PUBLIC(JNIEnv *) _cc_jni_onload(JavaVM *vm, pvoid_t reserved, jint version);

/**
 * @brief Get JNI File Mangager
 *
 * @return AssertManager Handle
 */
_CC_API_PUBLIC(AAssetManager *) _cc_jni_get_asset_manager(void);
/**
 * @brief Set JNI File Mangager
 *
 * @param asset_manager AAssetManager
 */
_CC_API_PUBLIC(void) _cc_jni_set_asset_manager(AAssetManager *asset_manager);
/**
 * @brief Power support
 *
 * @param plugged out param Plugged
 * @param charged out param Charged
 * @param battery out param Battery
 * @param seconds out param Seconds
 * @param percent out param Percent
 *
 * @return 0
 */
_CC_API_PUBLIC(int)
_cc_jni_get_power_info(int *plugged, int *charged, int *battery, int *seconds, byte_t *percent);
/**
 * @brief JNI Set clipboard Text
 *
 * @param text string
 *
 * @return 0
 */
_CC_API_PUBLIC(int) _cc_jni_set_clipboard_text(const tchar_t *text);
/**
 * @brief JNI Get clipboard Text
 *
 * @param str String buffer
 * @param len Maximum length of string buffer
 *
 * @return Length of string
 */
_CC_API_PUBLIC(int) _cc_jni_get_clipboard_text(tchar_t *str, int32_t len);
/**
 * @brief JNI Check the clipboard for strings
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_jni_has_clipboard_text(void);
/**
 * @brief JNI get the cache path
 *
 * @return Length of string
 */
_CC_API_PUBLIC(int) _cc_jni_get_cache_directory(tchar_t *str, int32_t len);
/**
 * @brief JNI get the files path
 *
 * @return Length of string
 */
_CC_API_PUBLIC(int) _cc_jni_get_files_directory(tchar_t *str, int32_t len);
/**
 * @brief JNI get the apk package name
 *
 * @return Length of string
 */
_CC_API_PUBLIC(int) _cc_jni_get_package_name(tchar_t *str, int32_t len);
/**
 * @brief JNI Check if it's an emulator
 *
 * @return true or FLASE
 */
_CC_API_PUBLIC(bool_t) _cc_jni_is_simulator(void);
/**
 * @brief JNI Open URL
 */
_CC_API_PUBLIC(int) _cc_jni_open_url(const char *url);
/**
 * @brief JNI Show toast notification
 */
_CC_API_PUBLIC(int)
_cc_jni_show_toast(const char *message, int duration, int gravity, int xOffset, int yOffset);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* __CC_ANDROID__ */

#endif /*_C_CC_ANDROID_H_INCLUDED_*/
