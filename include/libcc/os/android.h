#ifndef _C_CC_ANDROID_H_INCLUDED_
#define _C_CC_ANDROID_H_INCLUDED_

#include "../sds.h"

#include <android/native_window_jni.h>

//#ifdef __CC_ANDROID__

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
#define _CC_ANDROID_TAG_ "LIBCC-JNI"

typedef void (*RequestAndroidPermissionCallback_t)(void *userdata, const tchar_t *permission, bool granted);

// Interface from the SDL library into the Android Java activity
_CC_API_PUBLIC(void) Android_JNI_SetActivityTitle(const tchar_t *title);
_CC_API_PUBLIC(void) Android_JNI_SetWindowStyle(bool_t fullscreen);

_CC_API_PUBLIC(bool_t) IsAndroidTablet(void);
_CC_API_PUBLIC(bool_t) IsAndroidTV(void);
_CC_API_PUBLIC(bool_t) IsChromebook(void);
_CC_API_PUBLIC(bool_t) IsDeXMode(void);

_CC_API_PUBLIC(bool_t) Android_JNI_FileOpen(pvoid_t *puserdata, const tchar_t *fileName, const tchar_t *mode);
_CC_API_PUBLIC(int64_t) Android_JNI_FileSize(pvoid_t userdata);
_CC_API_PUBLIC(int64_t) Android_JNI_FileSeek(pvoid_t userdata, int64_t offset, int whence);
_CC_API_PUBLIC(size_t) Android_JNI_FileRead(pvoid_t userdata, pvoid_t buffer, size_t size);
_CC_API_PUBLIC(size_t) Android_JNI_FileWrite(pvoid_t userdata, const pvoid_t buffer, size_t size);
_CC_API_PUBLIC(bool_t) Android_JNI_FileFlush(pvoid_t userdata);
_CC_API_PUBLIC(bool_t) Android_JNI_FileClose(pvoid_t userdata);

// Environment support
_CC_API_PUBLIC(void) Android_JNI_GetManifestEnvironmentVariables(void);
_CC_API_PUBLIC(int) Android_JNI_OpenFileDescriptor(const tchar_t *uri, const tchar_t *mode);

// Clipboard support
_CC_API_PUBLIC(bool_t) Android_JNI_SetClipboardText(const tchar_t *text);
_CC_API_PUBLIC(int32_t) Android_JNI_GetClipboardText(char *text, size_t maxlen);
_CC_API_PUBLIC(bool_t) Android_JNI_HasClipboardText(void);

// Power support
_CC_API_PUBLIC(void) Android_JNI_GetPowerInfo(int *plugged, int *charged, int *battery, int *seconds, byte_t *percent);

// Threads
_CC_API_PUBLIC(JNIEnv*) Android_JNI_OnLoad(JavaVM *vm, void *reserved, jint version);
_CC_API_PUBLIC(JNIEnv*) Android_JNI_GetEnv(void);
_CC_API_PUBLIC(bool_t) Android_JNI_SetupThread(void);

// Locale
_CC_API_PUBLIC(bool_t) Android_JNI_GetLocale(tchar_t *buf, size_t buflen);

// Show toast notification
_CC_API_PUBLIC(bool_t) Android_JNI_ShowToast(const tchar_t *message, int duration, int gravity, int xOffset, int yOffset);

_CC_API_PUBLIC(bool_t) Android_JNI_OpenURL(const tchar_t *url);

_CC_API_PUBLIC(int) Android_JNI_GetSDKVersion(void);
_CC_API_PUBLIC(int) Android_JNI_GetNetworkType(void);
/**
 * See the official Android developer guide for more information:
 * http://developer.android.com/guide/topics/data/data-storage.html
 *
 */
#define CC_ANDROID_EXTERNAL_STORAGE_READ   0x01

/**
 * See the official Android developer guide for more information:
 * http://developer.android.com/guide/topics/data/data-storage.html
 *
 */
#define CC_ANDROID_EXTERNAL_STORAGE_WRITE  0x02

_CC_API_PUBLIC(uint32_t) Android_JNI_GetExternalStorageState(void);
_CC_API_PUBLIC(_cc_sds_t) Android_JNI_GetInternalStoragePath(void);
_CC_API_PUBLIC(_cc_sds_t) Android_JNI_GetExternalStoragePath(void);
_CC_API_PUBLIC(_cc_sds_t) Android_JNI_GetCachePath(void);

_CC_API_PUBLIC(bool_t) ShowAndroidToast(const tchar_t *message, int duration, int gravity, int xOffset, int yOffset);

_CC_API_PUBLIC(bool_t) Android_JNI_RequestPermission(const tchar_t *permission, RequestAndroidPermissionCallback_t cb, pvoid_t userdata);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

//#endif /* __CC_ANDROID__ */

#endif /*_C_CC_ANDROID_H_INCLUDED_*/
