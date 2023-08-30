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
#include <cc/atomic.h>
#include <cc/logger.h>
#include <cc/math.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __CC_ANDROID__
#include <android/configuration.h>
#include <cc/core/android.h>
#include <stdbool.h>

/*******************************************************************************
                               Globals
*******************************************************************************/
static JavaVM *_javaVM = NULL;
static pthread_key_t _thread_key;
static pthread_once_t _thread_key_once = PTHREAD_ONCE_INIT;

static AAssetManager *_asset_manager = NULL;
static jobject javaAssetManagerRef = 0;

/* Main activity */
static jclass _activity_class = 0;

/* method signatures */
static jmethodID mid_clipboard_get_text;
static jmethodID mid_clipboard_has_text;
static jmethodID mid_clipboard_set_text;
static jmethodID mid_is_simulator;
static jmethodID mid_register_power_info;
static jmethodID mid_get_cache_path;
static jmethodID mid_get_files_path;
static jmethodID mid_get_package_name;
static jmethodID mid_get_context;
static jmethodID mid_open_url;
static jmethodID mid_show_toast;

static struct _jni_power_info {
    int plugged;
    int charged;
    int battery;
    int seconds;
    int percent;
    bool_t registered;
} _android_power_info;

#ifndef _CC_JNI_VERSION_
#define _CC_JNI_VERSION_ JNI_VERSION_1_4
#endif

#define _CC_JNI_CONCAT(class, function) Java_org_libcc_##class##_##function
#define _CC_JNI_INTERFACE(function) _CC_JNI_CONCAT(CCWidgets, function)

#define _CC_JNI_GET_STATIC_METHOD_ID(_ENV, _CLS, _FUNC, _PARAMES)                                                      \
    (*(_ENV))->GetStaticMethodID(_ENV, _CLS, _FUNC, _PARAMES)

/* From http://developer.android.com/guide/practices/jni.html
 * All threads are Linux threads, scheduled by the kernel.
 * They're usually started from managed code (using Thread.start), but they can
 * also be created elsewhere and then attached to the JavaVM. For example, a
 * thread started with pthread_create can be attached with the JNI
 * AttachCurrentThread or AttachCurrentThreadAsDaemon functions. Until a thread
 * is attached, it has no JNIEnv, and cannot make JNI calls. Attaching a
 * natively-created thread causes a java.lang.Thread object to be constructed
 * and added to the "main" ThreadGroup, making it visible to the debugger.
 * Calling AttachCurrentThread on an already-attached thread is a no-op. Note:
 * You can call this function any number of times for the same thread, there's
 * no harm in it
 */

/* From http://developer.android.com/guide/practices/jni.html
 * Threads attached through JNI must call DetachCurrentThread before they exit.
 * If coding this directly is awkward, in Android 2.0 (Eclair) and higher you
 * can use pthread_key_create to define a destructor function that will be
 * called before the thread exits, and call DetachCurrentThread from there.
 * (Use that key with pthread_setspecific to store the JNIEnv in
 * thread-local-storage; that way it'll be passed into your destructor as the
 * argument.) Note: The destructor is not called unless the stored value is !=
 * NULL Note: You can call this function any number of times for the same
 * thread, there's no harm in it (except for some lost CPU cycles)
 */

/* Set local storage value */
static int Android_JNI_SetEnv(JNIEnv *env) {
    int status = pthread_setspecific(_thread_key, env);
    if (status < 0) {
        _cc_logger_error("Failed pthread_setspecific() in Android_JNI_SetEnv() (err=%d)", status);
    }
    return status;
}

/* Get local storage value */
JNIEnv *_cc_jni_get_env(void) {
    /* Get JNIEnv from the Thread local storage */
    JNIEnv *env = pthread_getspecific(_thread_key);
    if (env == NULL) {
        /* If it fails, try to attach ! (e.g the thread isn't created with
         * SDL_CreateThread() */
        int status;

        /* There should be a JVM */
        if (_javaVM == NULL) {
            _cc_logger_error("Failed, there is no JavaVM");
            return NULL;
        }

        /* Attach the current thread to the JVM and get a JNIEnv.
         * It will be detached by pthread_create destructor
         * 'Android_JNI_ThreadDestroyed' */
        status = (*_javaVM)->AttachCurrentThread(_javaVM, &env, NULL);
        if (status < 0) {
            _cc_logger_error("Failed to attach current thread (err=%d)", status);
            return NULL;
        }

        /* Save JNIEnv into the Thread local storage */
        if (Android_JNI_SetEnv(env) < 0) {
            return NULL;
        }
    }

    return env;
}

/* Set up an external thread for using JNI with _cc_jni_get_env() */
int _cc_jni_setup_thread(void) {
    JNIEnv *env;
    int status;

    /* There should be a JVM */
    if (_javaVM == NULL) {
        _cc_logger_error("Failed, there is no JavaVM");
        return 0;
    }

    /* Attach the current thread to the JVM and get a JNIEnv.
     * It will be detached by pthread_create destructor
     * 'Android_JNI_ThreadDestroyed' */
    status = (*_javaVM)->AttachCurrentThread(_javaVM, &env, NULL);
    if (status < 0) {
        _cc_logger_error("Failed to attach current thread (err=%d)", status);
        return 0;
    }

    /* Save JNIEnv into the Thread local storage */
    if (Android_JNI_SetEnv(env) < 0) {
        return 0;
    }

    return 1;
}

/* Destructor called for each thread where _thread_key is not NULL */
static void Android_JNI_ThreadDestroyed(void *value) {
    /* The thread is being destroyed, detach it from the Java VM and set the
     * _thread_key value to NULL as required */
    JNIEnv *env = (JNIEnv *)value;
    if (env != NULL) {
        (*_javaVM)->DetachCurrentThread(_javaVM);
        Android_JNI_SetEnv(NULL);
    }
}

/* Creation of local storage _thread_key */
static void Android_JNI_CreateKey(void) {
    int status = pthread_key_create(&_thread_key, Android_JNI_ThreadDestroyed);
    if (status < 0) {
        _cc_logger_error("Error initializing _thread_key with pthread_key_create() (err=%d)", status);
    }
}

static void Android_JNI_CreateKey_once(void) {
    int status = pthread_once(&_thread_key_once, Android_JNI_CreateKey);
    if (status < 0) {
        _cc_logger_error("Error initializing _thread_key with pthread_once() (err=%d)", status);
    }
}

/* Java class */
JNIEXPORT jboolean JNICALL _CC_JNI_INTERFACE(nativeSetupJNI)(JNIEnv *env, jclass jcls);

JNIEXPORT void JNICALL _CC_JNI_INTERFACE(setPowerInfo)(JNIEnv *env, jclass jcls, jint plugged, jint charged,
                                                       jint battery, jint seconds, jint percent);

static JNINativeMethod _cc_jni_native_methods[] = {{"nativeSetupJNI", "()Z", _CC_JNI_INTERFACE(nativeSetupJNI)},
                                                   {"setPowerInfo", "(IIIII)V", _CC_JNI_INTERFACE(setPowerInfo)}};

static void register_methods(JNIEnv *env, const char *classname, JNINativeMethod *methods, int nb) {
    jclass clazz = (*env)->FindClass(env, classname);
    if (clazz == NULL || (*env)->RegisterNatives(env, clazz, methods, nb) < 0) {
        _cc_logger_error("Failed to register methods of %s", classname);
        return;
    }
}

JNIEnv *_cc_jni_onload(JavaVM *vm, void *reserved, jint version) {
    JNIEnv *env = NULL;

    _cc_logger_debug("JNI_OnLoad init");

    bzero(&_android_power_info, sizeof(_android_power_info));
    _android_power_info.registered = false;

    _javaVM = vm;

    if ((*_javaVM)->GetEnv(_javaVM, (void **)&env, version) != JNI_OK) {
        _cc_logger_error("Failed to get JNI Env");
        return NULL;
    }

    register_methods(env, "org/libcc/CCWidgets", _cc_jni_native_methods, _cc_countof(_cc_jni_native_methods));

    return env;
}

/*******************************************************************************
                 Functions called by JNI
*******************************************************************************/
JNIEXPORT void JNICALL _CC_JNI_INTERFACE(setPowerInfo)(JNIEnv *env, jclass cls, jint plugged, jint charged,
                                                       jint battery, jint seconds, jint percent) {
    _android_power_info.plugged = plugged;
    _android_power_info.charged = charged;
    _android_power_info.battery = battery;
    _android_power_info.seconds = seconds;
    _android_power_info.percent = percent;
}

/* Activity initialization */
JNIEXPORT jboolean JNICALL _CC_JNI_INTERFACE(nativeSetupJNI)(JNIEnv *env, jclass cls) {
    _cc_logger_debug("nativeSetupJNI()");

    /*
     * Create _thread_key so we can keep track of the JNIEnv assigned to each
     * thread Refer to
     * http://developer.android.com/guide/practices/design/jni.html for the
     * rationale behind this
     */
    Android_JNI_CreateKey_once();

    /* Save JNIEnv of CCWidgets */
    Android_JNI_SetEnv(env);

    _activity_class = (jclass)((*env)->NewGlobalRef(env, cls));

    mid_get_context = _CC_JNI_GET_STATIC_METHOD_ID(env, _activity_class, "getContext", "()Landroid/content/Context;");

    mid_clipboard_get_text =
        _CC_JNI_GET_STATIC_METHOD_ID(env, _activity_class, "clipboardGetText", "()Ljava/lang/String;");
    mid_clipboard_has_text = _CC_JNI_GET_STATIC_METHOD_ID(env, _activity_class, "clipboardHasText", "()Z");
    mid_clipboard_set_text =
        _CC_JNI_GET_STATIC_METHOD_ID(env, _activity_class, "clipboardSetText", "(Ljava/lang/String;)V");

    mid_is_simulator = _CC_JNI_GET_STATIC_METHOD_ID(env, _activity_class, "isSimulator", "()Z");
    mid_register_power_info = _CC_JNI_GET_STATIC_METHOD_ID(env, _activity_class, "registerPowerInfo", "()V");

    mid_get_cache_path = _CC_JNI_GET_STATIC_METHOD_ID(env, _activity_class, "getCachePath", "()Ljava/lang/String;");
    mid_get_files_path = _CC_JNI_GET_STATIC_METHOD_ID(env, _activity_class, "getFilesPath", "()Ljava/lang/String;");

    mid_get_package_name = _CC_JNI_GET_STATIC_METHOD_ID(env, _activity_class, "getPackageName", "()Ljava/lang/String;");

    mid_open_url = _CC_JNI_GET_STATIC_METHOD_ID(env, _activity_class, "openURL", "(Ljava/lang/String;)I");
    mid_show_toast = _CC_JNI_GET_STATIC_METHOD_ID(env, _activity_class, "showToast", "(Ljava/lang/String;IIII)I");

    if (!mid_clipboard_get_text || !mid_clipboard_has_text || !mid_clipboard_set_text || !mid_register_power_info ||
        !mid_get_cache_path || !mid_get_files_path || !mid_get_package_name || !mid_get_context || !mid_open_url ||
        !mid_show_toast || !mid_is_simulator) {
        _cc_logger_error("Missing some Java callbacks, do you have the latest version of CC.java?");
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

/*******************************************************************************
             Functions called by CC into Java
*******************************************************************************/
static _cc_atomic32_t local_reference_active;
typedef struct LocalReferenceHolder {
    JNIEnv *env;
    const char *func;
} LocalReferenceHolder;

static LocalReferenceHolder LocalReferenceHolder_Setup(const char *func) {
    LocalReferenceHolder refholder;
    refholder.env = NULL;
    refholder.func = func;
    return refholder;
}

static bool_t LocalReferenceHolder_Init(LocalReferenceHolder *refholder, JNIEnv *env) {
    const int capacity = 16;
    if ((*env)->PushLocalFrame(env, capacity) < 0) {
        _cc_logger_error("Failed to allocate enough JVM local references");
        return false;
    }

    _cc_atomic32_inc(&local_reference_active);
    refholder->env = env;

    return true;
}

static void LocalReferenceHolder_Cleanup(LocalReferenceHolder *refholder) {
    if (refholder->env) {
        JNIEnv *env = refholder->env;
        (*env)->PopLocalFrame(env, NULL);
        _cc_atomic32_dec(&local_reference_active);
    }
}

static void AndroidCreateAssetManager() {
    LocalReferenceHolder refs = LocalReferenceHolder_Setup(__FUNCTION__);
    JNIEnv *env = _cc_jni_get_env();
    jmethodID mid;
    jobject context;
    jobject javaAssetManager;

    if (!LocalReferenceHolder_Init(&refs, env)) {
        LocalReferenceHolder_Cleanup(&refs);
        return;
    }

    context = (*env)->CallStaticObjectMethod(env, _activity_class, mid_get_context);

    /* javaAssetManager = context.getAssets(); */
    mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getAssets", "()Landroid/content/res/AssetManager;");
    javaAssetManager = (*env)->CallObjectMethod(env, context, mid);

    /**
     * Given a Dalvik AssetManager object, obtain the corresponding native
     * AAssetManager object.  Note that the caller is responsible for obtaining
     * and holding a VM reference to the jobject to prevent its being garbage
     * collected while the native object is in use.
     */
    javaAssetManagerRef = (*env)->NewGlobalRef(env, javaAssetManager);
    _asset_manager = AAssetManager_fromJava(env, javaAssetManagerRef);

    if (_asset_manager == NULL) {
        (*env)->DeleteGlobalRef(env, javaAssetManagerRef);
    }

    LocalReferenceHolder_Cleanup(&refs);
}

static void AndroidDestroyAssetManager() {
    JNIEnv *env = _cc_jni_get_env();

    if (_asset_manager) {
        (*env)->DeleteGlobalRef(env, javaAssetManagerRef);
        _asset_manager = NULL;
    }
}

AAssetManager *_cc_jni_get_asset_manager(void) {
    if (_asset_manager == NULL) {
        AndroidCreateAssetManager();
    }
    return _asset_manager;
}

void _cc_jni_set_asset_manager(AAssetManager *asset_manager) {
    _asset_manager = asset_manager;
}

bool_t _cc_jni_get_locale(char *buf, size_t buflen) {
    AConfiguration *cfg;

    _cc_assert(buflen > 6);
    /* Need to re-create the asset manager if locale has changed (SDL_LOCALECHANGED) */
    AndroidDestroyAssetManager();

    if (_asset_manager == NULL) {
        AndroidCreateAssetManager();
    }

    if (_asset_manager == NULL) {
        return false;
    }

    cfg = AConfiguration_new();
    if (cfg == NULL) {
        return false;
    }

    {
        char language[2] = {};
        char country[2] = {};
        size_t id = 0;

        AConfiguration_fromAssetManager(cfg, _asset_manager);
        AConfiguration_getLanguage(cfg, language);
        AConfiguration_getCountry(cfg, country);

        /* copy language (not null terminated) */
        if (language[0]) {
            buf[id++] = language[0];
            if (language[1]) {
                buf[id++] = language[1];
            }
        }

        buf[id++] = '_';

        /* copy country (not null terminated) */
        if (country[0]) {
            buf[id++] = country[0];
            if (country[1]) {
                buf[id++] = country[1];
            }
        }

        buf[id++] = '\0';
        _cc_assert(id <= buflen);
    }

    AConfiguration_delete(cfg);

    return true;
}

int _cc_jni_set_clipboard_text(const tchar_t *text) {
    JNIEnv *env = _cc_jni_get_env();
    jstring string = (*env)->NewStringUTF(env, text);

    (*env)->CallStaticVoidMethod(env, _activity_class, mid_clipboard_set_text, string);
    (*env)->DeleteLocalRef(env, string);

    return 0;
}

bool_t _cc_jni_is_simulator(void) {
    JNIEnv *env = _cc_jni_get_env();
    return (*env)->CallStaticBooleanMethod(env, _activity_class, mid_is_simulator);
}

int _cc_jni_get_clipboard_text(tchar_t *str, int32_t len) {
    jstring string;
    JNIEnv *env = _cc_jni_get_env();

    string = (*env)->CallStaticObjectMethod(env, _activity_class, mid_clipboard_get_text);
    if (string) {
        const char *utf = (*env)->GetStringUTFChars(env, string, 0);
        if (utf) {
            int utf_len = _cc_min_int32(len - 1, (*env)->GetStringUTFLength(env, string));
            strncpy(str, utf, utf_len);
            str[utf_len] = 0;

            (*env)->ReleaseStringUTFChars(env, string, utf);
            return utf_len;
        }
        (*env)->DeleteLocalRef(env, string);
    }

    return 0;
}

int _cc_jni_get_cache_directory(tchar_t *str, int32_t len) {
    jstring string;
    JNIEnv *env = _cc_jni_get_env();

    string = (*env)->CallStaticObjectMethod(env, _activity_class, mid_get_cache_path);
    if (string) {
        const char *utf = (*env)->GetStringUTFChars(env, string, 0);
        if (utf) {
            int utf_len = _cc_min_int32(len - 1, (*env)->GetStringUTFLength(env, string));
            strncpy(str, utf, utf_len);
            str[utf_len] = 0;

            (*env)->ReleaseStringUTFChars(env, string, utf);
            return utf_len;
        }
        (*env)->DeleteLocalRef(env, string);
    }

    return 0;
}

int _cc_jni_get_files_directory(tchar_t *str, int32_t len) {
    jstring string;
    JNIEnv *env = _cc_jni_get_env();

    string = (*env)->CallStaticObjectMethod(env, _activity_class, mid_get_files_path);
    if (string) {
        const char *utf = (*env)->GetStringUTFChars(env, string, 0);
        if (utf) {
            int utf_len = _cc_min_int32(len - 1, (*env)->GetStringUTFLength(env, string));
            strncpy(str, utf, utf_len);
            str[utf_len] = 0;

            (*env)->ReleaseStringUTFChars(env, string, utf);
            return utf_len;
        }
        (*env)->DeleteLocalRef(env, string);
    }

    return 0;
}
int _cc_jni_get_package_name(tchar_t *str, int32_t len) {
    jstring string;
    JNIEnv *env = _cc_jni_get_env();

    string = (*env)->CallStaticObjectMethod(env, _activity_class, mid_get_package_name);
    if (string) {
        const char *utf = (*env)->GetStringUTFChars(env, string, 0);
        if (utf) {
            int utf_len = _cc_min_int32(len - 1, (*env)->GetStringUTFLength(env, string));
            strncpy(str, utf, utf_len);
            str[utf_len] = 0;

            (*env)->ReleaseStringUTFChars(env, string, utf);
            return utf_len;
        }
        (*env)->DeleteLocalRef(env, string);
    }

    return 0;
}

bool_t _cc_jni_has_clipboard_text(void) {
    JNIEnv *env = _cc_jni_get_env();
    jboolean has = (*env)->CallStaticBooleanMethod(env, _activity_class, mid_clipboard_has_text);
    return (has == JNI_TRUE);
}

/*
int _cc_jni_get_android_sdk_version(void) {
    static int sdk_version;
    if (!sdk_version) {
        char sdk[PROP_VALUE_MAX] = {0};
        if (_system_property_get("ro.build.version.sdk", sdk) != 0) {
            sdk_version = atoi(sdk);
        }
    }
    return sdk_version;
}
*/

int _cc_jni_get_power_info(int *plugged, int *charged, int *battery, int *seconds, byte_t *percent) {
    JNIEnv *env = _cc_jni_get_env();

    if (!_android_power_info.registered) {
        (*env)->CallStaticVoidMethod(env, _activity_class, mid_register_power_info);
        _android_power_info.registered = true;
    }

    if (plugged) {
        *plugged = _android_power_info.plugged;
    }

    if (charged) {
        *charged = _android_power_info.charged;
    }

    if (battery) {
        *battery = _android_power_info.battery;
    }

    if (seconds) {
        *seconds = _android_power_info.seconds;
    }

    if (percent) {
        *percent = (byte_t)_android_power_info.percent;
    }

    return 0;
}

int _cc_jni_open_url(const char *url) {
    int result = 0;
    JNIEnv *env = _cc_jni_get_env();
    jstring jurl = (*env)->NewStringUTF(env, url);
    result = (*env)->CallStaticIntMethod(env, _activity_class, mid_open_url);
    (*env)->DeleteLocalRef(env, jurl);
    return result;
}

/* Show toast notification */
int _cc_jni_show_toast(const char *message, int duration, int gravity, int xOffset, int yOffset) {
    int result = 0;
    JNIEnv *env = _cc_jni_get_env();
    jstring jmessage = (*env)->NewStringUTF(env, message);
    result = (*env)->CallStaticIntMethod(env, _activity_class, mid_show_toast, jmessage, duration, gravity, xOffset, yOffset);
    (*env)->DeleteLocalRef(env, jmessage);
    return result;
}

#endif /* __CC_ANDROID__ */