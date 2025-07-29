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
#include <libcc/version.h>
#include <libcc/alloc.h>
#include <libcc/mutex.h>
#include <libcc/atomic.h>
#include <libcc/math.h>
#include <libcc/core/android.h>
#include <sys/errno.h>

#include <android/log.h>
#include <android/configuration.h>
#include <android/asset_manager_jni.h>
#include <sys/system_properties.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <dlfcn.h>

#define _CC_JAVA_PREFIX_                                cn_libcc
#define _CONCAT1(prefix, class, function)               _CONCAT2(prefix, class, function)
#define _CONCAT2(prefix, class, function)               Java_##prefix##_##class##_##function
#define CC_JAVA_INTERFACE(function)                     _CONCAT1(_CC_JAVA_PREFIX_, CCWidgets, function)

// Java class CCWidgets
JNIEXPORT jstring JNICALL CC_JAVA_INTERFACE(nativeGetVersion)(
    JNIEnv *env, jclass cls);

JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativeSetupJNI)(
    JNIEnv *env, jclass cls);

JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativeQuit)(
    JNIEnv *env, jclass cls);

JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativePause)(
    JNIEnv *env, jclass cls);

JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativeResume)(
    JNIEnv *env, jclass cls);

JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativePermissionResult)(
    JNIEnv *env, jclass cls,
    jint requestCode, jboolean result);

JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativeFocusChanged)(
    JNIEnv *env, jclass cls, jboolean hasFocus);

JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativeDropFile)(
    JNIEnv *env, jclass jcls,
    jstring filename);

JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativeLocaleChanged)(
    JNIEnv *env, jclass cls);

JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativeDarkModeChanged)(
    JNIEnv *env, jclass cls, jboolean enabled);

JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativeClipboardChanged)(
    JNIEnv *env, jclass jcls);

JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativeSetPowerInfo)(
    JNIEnv *env, jclass jcls, jint plugged, jint charged,jint battery, jint seconds, jint percent);

static JNINativeMethod CCWidgets_tab[] = {
    { "nativeGetVersion", "()Ljava/lang/String;", CC_JAVA_INTERFACE(nativeGetVersion) },
    { "nativeSetupJNI", "()I", CC_JAVA_INTERFACE(nativeSetupJNI) },
    { "nativeClipboardChanged", "()V", CC_JAVA_INTERFACE(nativeClipboardChanged) },
    { "nativeLocaleChanged", "()V", CC_JAVA_INTERFACE(nativeLocaleChanged) },
    { "nativeDarkModeChanged", "(Z)V", CC_JAVA_INTERFACE(nativeDarkModeChanged) },
    { "nativeDropFile", "(Ljava/lang/String;)V", CC_JAVA_INTERFACE(nativeDropFile) },
    { "nativeQuit", "()V", CC_JAVA_INTERFACE(nativeQuit) },
    { "nativePause", "()V", CC_JAVA_INTERFACE(nativePause) },
    { "nativeResume", "()V", CC_JAVA_INTERFACE(nativeResume) },
    { "nativeFocusChanged", "(Z)V", CC_JAVA_INTERFACE(nativeFocusChanged) },
    { "nativePermissionResult", "(IZ)V", CC_JAVA_INTERFACE(nativePermissionResult) },
    { "nativeSetPowerInfo", "(IIIII)V", CC_JAVA_INTERFACE(nativeSetPowerInfo)}
};


// Uncomment this to log messages entering and exiting methods in this file
// #define DEBUG_JNI

/*******************************************************************************
 This file links the Java side of Android with libcc
*******************************************************************************/
#include <jni.h>

/*******************************************************************************
                               Globals
*******************************************************************************/
static pthread_key_t mThreadKey;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static JavaVM *mJavaVM = nullptr;
static _cc_mutex_t *Android_ActivityMutex = nullptr;

static _cc_String_t s_AndroidInternalFilesPath = {0};
static _cc_String_t s_AndroidExternalFilesPath = {0};
static _cc_String_t s_AndroidCachePath = {0};

// Main activity
static jclass mWidgetsClass;

// method signatures
static jmethodID midClipboardGetText;
static jmethodID midClipboardHasText;
static jmethodID midClipboardSetText;
static jmethodID midGetContext;
static jmethodID midGetManifestEnvironmentVariables;
static jmethodID midIsAndroidTV;
static jmethodID midIsChromebook;
static jmethodID midIsDeXMode;
static jmethodID midIsTablet;
static jmethodID midOpenURL;
static jmethodID midRequestPermission;
static jmethodID midShowToast;
static jmethodID midSetActivityTitle;
static jmethodID midSetWindowStyle;
static jmethodID midGetPreferredLocales;
static jmethodID midOpenFileDescriptor;
static jmethodID midRegisterPowerInfo;

static bool_t bHasEnvironmentVariables = false;

// Android AssetManager
static void Internal_Android_Create_AssetManager(void);
static void Internal_Android_Destroy_AssetManager(void);
static AAssetManager *asset_manager = nullptr;
static jobject javaAssetManagerRef = 0;

static struct {
    int plugged;
    int charged;
    int battery;
    int seconds;
    int percent;
    bool_t registered;
} androidPowerInfo;

/*******************************************************************************
                 Functions called by JNI
*******************************************************************************/

/* From http://developer.android.com/guide/practices/jni.html
 * All threads are Linux threads, scheduled by the kernel.
 * They're usually started from managed code (using Thread.start), but they can also be created elsewhere and then
 * attached to the JavaVM. For example, a thread started with pthread_create can be attached with the
 * JNI AttachCurrentThread or AttachCurrentThreadAsDaemon functions. Until a thread is attached, it has no JNIEnv,
 * and cannot make JNI calls.
 * Attaching a natively-created thread causes a java.lang.Thread object to be constructed and added to the "main"
 * ThreadGroup, making it visible to the debugger. Calling AttachCurrentThread on an already-attached thread
 * is a no-op.
 * Note: You can call this function any number of times for the same thread, there's no harm in it
 */

/* From http://developer.android.com/guide/practices/jni.html
 * Threads attached through JNI must call DetachCurrentThread before they exit. If coding this directly is awkward,
 * in Android 2.0 (Eclair) and higher you can use pthread_key_create to define a destructor function that will be
 * called before the thread exits, and call DetachCurrentThread from there. (Use that key with pthread_setspecific
 * to store the JNIEnv in thread-local-storage; that way it'll be passed into your destructor as the argument.)
 * Note: The destructor is not called unless the stored value is != nullptr
 * Note: You can call this function any number of times for the same thread, there's no harm in it
 *       (except for some lost CPU cycles)
 */

// Set local storage value
_CC_API_PRIVATE(bool_t) Android_JNI_SetEnv(JNIEnv *env) {
    int status = pthread_setspecific(mThreadKey, env);
    if (status < 0) {
        __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "Failed pthread_setspecific() in Android_JNI_SetEnv() (err=%d)", status);
        return false;
    }
    return true;
}

// Get local storage value
_CC_API_PUBLIC(JNIEnv*) Android_JNI_GetEnv(void) {
    // Get JNIEnv from the Thread local storage
    JNIEnv *env = pthread_getspecific(mThreadKey);
    if (!env) {
        // If it fails, try to attach ! (e.g the thread isn't created with CC_CreateThread()
        int status;

        // There should be a JVM
        if (!mJavaVM) {
            __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "Failed, there is no JavaVM");
            return nullptr;
        }

        /* Attach the current thread to the JVM and get a JNIEnv.
         * It will be detached by pthread_create destructor 'Android_JNI_ThreadDestroyed' */
        status = (*mJavaVM)->AttachCurrentThread(mJavaVM, &env, nullptr);
        if (status < 0) {
            __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "Failed to attach current thread (err=%d)", status);
            return nullptr;
        }

        // Save JNIEnv into the Thread local storage
        if (!Android_JNI_SetEnv(env)) {
            return nullptr;
        }
    }

    return env;
}

// Set up an external thread for using JNI with Android_JNI_GetEnv()
_CC_API_PUBLIC(bool_t) Android_JNI_SetupThread(void) {
    JNIEnv *env;
    int status;

    // There should be a JVM
    if (!mJavaVM) {
        __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "Failed, there is no JavaVM");
        return false;
    }

    /* Attach the current thread to the JVM and get a JNIEnv.
     * It will be detached by pthread_create destructor 'Android_JNI_ThreadDestroyed' */
    status = (*mJavaVM)->AttachCurrentThread(mJavaVM, &env, nullptr);
    if (status < 0) {
        __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "Failed to attach current thread (err=%d)", status);
        return false;
    }

    // Save JNIEnv into the Thread local storage
    if (!Android_JNI_SetEnv(env)) {
        return false;
    }

    return true;
}

// Destructor called for each thread where mThreadKey is not nullptr
_CC_API_PRIVATE(void) Android_JNI_ThreadDestroyed(pvoid_t value) {
    // The thread is being destroyed, detach it from the Java VM and set the mThreadKey value to nullptr as required
    JNIEnv *env = (JNIEnv *)value;
    if (env) {
        (*mJavaVM)->DetachCurrentThread(mJavaVM);
        Android_JNI_SetEnv(nullptr);
    }
}

// Creation of local storage mThreadKey
_CC_API_PRIVATE(void) Android_JNI_CreateKey(void) {
    int status = pthread_key_create(&mThreadKey, Android_JNI_ThreadDestroyed);
    if (status < 0) {
        __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "Error initializing mThreadKey with pthread_key_create() (err=%d)", status);
    }
}

_CC_API_PRIVATE(void) Android_JNI_CreateKey_once(void) {
    int status = pthread_once(&key_once, Android_JNI_CreateKey);
    if (status < 0) {
        __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "Error initializing mThreadKey with pthread_once() (err=%d)", status);
    }
}

_CC_API_PRIVATE(void) register_methods(JNIEnv *env, const char *classname, JNINativeMethod *methods, int nb) {
    jclass clazz = (*env)->FindClass(env, classname);
    if (!clazz || (*env)->RegisterNatives(env, clazz, methods, nb) < 0) {
        __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "Failed to register methods of %s", classname);
        return;
    }
}

_CC_API_PUBLIC(JNIEnv*) Android_JNI_OnLoad(JavaVM *vm, void *reserved, jint version) {
    JNIEnv *env = nullptr;

    s_AndroidInternalFilesPath.length = 0;
    s_AndroidExternalFilesPath.length = 0;
    s_AndroidCachePath.length = 0;
    
    _cc_logger_debug("Android_JNI_OnLoad");

    mJavaVM = vm;

    if ((*mJavaVM)->GetEnv(mJavaVM, (pvoid_t *)&env, version) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "Failed to get JNI Env");
        return nullptr;
    }

    register_methods(env, "org/libcc/CCWidgets", CCWidgets_tab, _cc_countof(CCWidgets_tab));
    return env;

}

// Get CC version -- called before cc_main() to verify JNI bindings
JNIEXPORT jstring JNICALL CC_JAVA_INTERFACE(nativeGetVersion)(JNIEnv *env, jclass cls) {
    char version[128];

    //_tsnprintf(version, sizeof(version), _T("%d.%d.%d"), _CC_MAJOR_VERSION_, _CC_MINOR_VERSION_, _CC_MICRO_VERSION_);

    return (*env)->NewStringUTF(env, _CC_VERSION_);
}

// Activity initialization -- called before cc_main() to initialize JNI bindings
JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativeSetupJNI)(JNIEnv *env, jclass cls) {
    __android_log_print(ANDROID_LOG_VERBOSE, _CC_ANDROID_TAG_, "nativeSetupJNI()");

    // Start with a clean slate
    //ClearError();

    androidPowerInfo.registered = false;

    if (!Android_ActivityMutex) {
        Android_ActivityMutex = _cc_alloc_mutex(); // Could this be created twice if onCreate() is called a second time ?
    }
    /*
     * Create mThreadKey so we can keep track of the JNIEnv assigned to each thread
     * Refer to http://developer.android.com/guide/practices/design/jni.html for the rationale behind this
     */
    Android_JNI_CreateKey_once();

    // Save JNIEnv of CCWidgets
    Android_JNI_SetEnv(env);

    if (!mJavaVM) {
        __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "failed to found a JavaVM");
    }

    mWidgetsClass = (jclass)((*env)->NewGlobalRef(env, cls));

    midClipboardGetText = (*env)->GetStaticMethodID(env, mWidgetsClass, "clipboardGetText", "()Ljava/lang/String;");
    midClipboardHasText = (*env)->GetStaticMethodID(env, mWidgetsClass, "clipboardHasText", "()Z");
    midClipboardSetText = (*env)->GetStaticMethodID(env, mWidgetsClass, "clipboardSetText", "(Ljava/lang/String;)V");
    midGetContext = (*env)->GetStaticMethodID(env, mWidgetsClass, "getContext", "()Landroid/content/Context;");

    midGetManifestEnvironmentVariables = (*env)->GetStaticMethodID(env, mWidgetsClass, "getManifestEnvironmentVariables", "()Z");

    midIsAndroidTV = (*env)->GetStaticMethodID(env, mWidgetsClass, "isAndroidTV", "()Z");
    midIsChromebook = (*env)->GetStaticMethodID(env, mWidgetsClass, "isChromebook", "()Z");
    midIsDeXMode = (*env)->GetStaticMethodID(env, mWidgetsClass, "isDeXMode", "()Z");
    midIsTablet = (*env)->GetStaticMethodID(env, mWidgetsClass, "isTablet", "()Z");

    midOpenURL = (*env)->GetStaticMethodID(env, mWidgetsClass, "openURL", "(Ljava/lang/String;)Z");
    midRequestPermission = (*env)->GetStaticMethodID(env, mWidgetsClass, "requestPermission", "(Ljava/lang/String;I)V");

    midShowToast = (*env)->GetStaticMethodID(env, mWidgetsClass, "showToast", "(Ljava/lang/String;IIII)Z");
    midSetWindowStyle = (*env)->GetStaticMethodID(env, mWidgetsClass, "setWindowStyle", "(Z)V");
    midSetActivityTitle = (*env)->GetStaticMethodID(env, mWidgetsClass, "setActivityTitle", "(Ljava/lang/String;)Z");
    midGetPreferredLocales = (*env)->GetStaticMethodID(env, mWidgetsClass, "getPreferredLocales", "()Ljava/lang/String;");
    midOpenFileDescriptor = (*env)->GetStaticMethodID(env, mWidgetsClass, "openFileDescriptor", "(Ljava/lang/String;Ljava/lang/String;)I");

    midRegisterPowerInfo = (*env)->GetStaticMethodID(env, mWidgetsClass, "registerPowerInfo", "()V");

    if (!midClipboardGetText ||
        !midClipboardHasText ||
        !midClipboardSetText ||
        !midGetContext ||
        !midGetManifestEnvironmentVariables ||
        !midIsAndroidTV ||
        !midIsChromebook ||
        !midIsDeXMode ||
        !midIsTablet ||
        !midOpenURL ||
        !midRequestPermission ||
        !midShowToast ||
        !midSetActivityTitle ||
        !midSetWindowStyle ||
        !midOpenFileDescriptor ||
        !midRegisterPowerInfo ||
        !midGetPreferredLocales) {
        __android_log_print(ANDROID_LOG_WARN, _CC_ANDROID_TAG_, "Missing some Java callbacks, do you have the latest version of CCWidgets.java?");
    }
}

// Drop file
JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativeDropFile)(
    JNIEnv *env, jclass jcls,
    jstring filename) {
    const char *path = (*env)->GetStringUTFChars(env, filename, nullptr);
    __android_log_print(ANDROID_LOG_VERBOSE, _CC_ANDROID_TAG_, "nativeDropFile(%s)",path);
    //SDL_SendDropFile(nullptr, nullptr, path);
    (*env)->ReleaseStringUTFChars(env, filename, path);
    //SendDropComplete(nullptr);
}

// Clipboard
JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativeClipboardChanged)(
    JNIEnv *env, jclass jcls) {
    // TODO: compute new mime types
    //SendClipboardUpdate(false, nullptr, 0);
    __android_log_print(ANDROID_LOG_VERBOSE, _CC_ANDROID_TAG_, "nativeClipboardChanged()");
}

/* Locale
 * requires android:configChanges="layoutDirection|locale" in AndroidManifest.xml */
JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativeLocaleChanged)(
    JNIEnv *env, jclass cls) {
    //SendAppEvent(CC_EVENT_LOCALE_CHANGED);
    __android_log_print(ANDROID_LOG_VERBOSE, _CC_ANDROID_TAG_, "nativeLocaleChanged()");
}

// Dark mode
JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativeDarkModeChanged)(
    JNIEnv *env, jclass cls, jboolean enabled) {
    //Android_SetDarkMode(enabled);
    __android_log_print(ANDROID_LOG_VERBOSE, _CC_ANDROID_TAG_, "nativeDarkModeChanged()");
}

// Activity ends
JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativeQuit)(
    JNIEnv *env, jclass cls) {
    //const char *str;

    __android_log_print(ANDROID_LOG_VERBOSE, _CC_ANDROID_TAG_, "nativeQuit()");

    if (Android_ActivityMutex) {
        _cc_free_mutex(Android_ActivityMutex);
    }
    _cc_safe_free(s_AndroidInternalFilesPath.data);
    _cc_safe_free(s_AndroidExternalFilesPath.data);
    _cc_safe_free(s_AndroidCachePath.data);

    s_AndroidInternalFilesPath.length = 0;
    s_AndroidExternalFilesPath.length = 0;
    s_AndroidCachePath.length = 0;
    Internal_Android_Destroy_AssetManager();


    // str = _cc_last_error(_cc_last_errno());
    // if (str && str[0]) {
    //     __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "CCWidgets thread ends (error=%s)", str);
    // } else {
    //     __android_log_print(ANDROID_LOG_VERBOSE, _CC_ANDROID_TAG_, "CCWidgets thread ends");
    // }
}

// Pause
JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativePause)(
    JNIEnv *env, jclass cls) {
    __android_log_print(ANDROID_LOG_VERBOSE, _CC_ANDROID_TAG_, "nativePause()");
}

// Resume
JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativeResume)(
    JNIEnv *env, jclass cls) {
    __android_log_print(ANDROID_LOG_VERBOSE, _CC_ANDROID_TAG_, "nativeResume()");
}

JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativeFocusChanged)(
    JNIEnv *env, jclass cls, jboolean hasFocus) {
    __android_log_print(ANDROID_LOG_VERBOSE, _CC_ANDROID_TAG_, "nativeFocusChanged()");
}

JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativeSetPowerInfo)(
    JNIEnv *env, jclass cls, jint plugged, jint charged, jint battery, jint seconds, jint percent) {
    androidPowerInfo.plugged = plugged;
    androidPowerInfo.charged = charged;
    androidPowerInfo.battery = battery;
    androidPowerInfo.seconds = seconds;
    androidPowerInfo.percent = percent;
}
/*
JNIEXPORT jstring JNICALL CC_JAVA_INTERFACE(nativeGetHint)(
    JNIEnv *env, jclass cls,
    jstring name) {
    const char *utfname = (*env)->GetStringUTFChars(env, name, nullptr);
    //const char *hint = CC_GetHint(utfname);

    jstring result = (*env)->NewStringUTF(env, hint);
    (*env)->ReleaseStringUTFChars(env, name, utfname);

    return result;
}

JNIEXPORT jboolean JNICALL CC_JAVA_INTERFACE(nativeGetHintBoolean)(
    JNIEnv *env, jclass cls,
    jstring name, jboolean default_value) {
    jboolean result;

    const char *utfname = (*env)->GetStringUTFChars(env, name, nullptr);
    //result = CC_GetHintBoolean(utfname, default_value);
    (*env)->ReleaseStringUTFChars(env, name, utfname);

    return result;
}
*/
/*******************************************************************************
             Functions called by CC into Java
*******************************************************************************/

static _cc_atomic32_t s_active;
struct LocalReferenceHolder {
    JNIEnv *m_env;
    const char *m_func;
};

_CC_API_PRIVATE(struct LocalReferenceHolder) LocalReferenceHolder_Setup(const char *func) {
    struct LocalReferenceHolder refholder;
    refholder.m_env = nullptr;
    refholder.m_func = func;
#ifdef _CC_DEBUG_
    _cc_logger_debug(_T("Entering function %s"), func);
#endif
    return refholder;
}

_CC_API_PRIVATE(bool_t) LocalReferenceHolder_Init(struct LocalReferenceHolder *refholder, JNIEnv *env) {
    const int capacity = 16;
    if ((*env)->PushLocalFrame(env, capacity) < 0) {
        _cc_logger_error(_T("Failed to allocate enough JVM local references"));
        return false;
    }
    _cc_atomic32_inc(&s_active);
    refholder->m_env = env;
    return true;
}

_CC_API_PRIVATE(void) LocalReferenceHolder_Cleanup(struct LocalReferenceHolder *refholder) {
#ifdef _CC_DEBUG_
    _cc_logger_debug(_T("Leaving function %s"), refholder->m_func);
#endif
    if (refholder->m_env) {
        JNIEnv *env = refholder->m_env;
        (*env)->PopLocalFrame(env, nullptr);
        _cc_atomic32_dec(&s_active);
    }
}

_CC_API_PUBLIC(void) Android_JNI_SetActivityTitle(const tchar_t *title) {
    JNIEnv *env = Android_JNI_GetEnv();

    jstring jtitle = (*env)->NewStringUTF(env, title);
    (*env)->CallStaticBooleanMethod(env, mWidgetsClass, midSetActivityTitle, jtitle);
    (*env)->DeleteLocalRef(env, jtitle);
}

_CC_API_PUBLIC(void) Android_JNI_SetWindowStyle(bool_t fullscreen) {
    JNIEnv *env = Android_JNI_GetEnv();
    (*env)->CallStaticVoidMethod(env, mWidgetsClass, midSetWindowStyle, fullscreen ? 1 : 0);
}

// Test for an exception and call SetError with its detail if one occurs
// If the parameter silent is truthy then SetError() will not be called.
_CC_API_PRIVATE(bool_t) Android_JNI_ExceptionOccurred(bool_t silent) {
    JNIEnv *env = Android_JNI_GetEnv();
    jthrowable exception;

    // Detect mismatch LocalReferenceHolder_Init/Cleanup
    __sync_or_and_fetch(&s_active, 0);
    //_cc_assert(GetAtomicInt(&s_active) > 0);

    exception = (*env)->ExceptionOccurred(env);
    if (exception != nullptr) {
        jmethodID mid;

        // Until this happens most JNI operations have undefined behaviour
        (*env)->ExceptionClear(env);

        if (!silent) {
            jclass exceptionClass = (*env)->GetObjectClass(env, exception);
            jclass classClass = (*env)->FindClass(env, "java/lang/Class");
            jstring exceptionName;
            const char *exceptionNameUTF8;
            jstring exceptionMessage;

            mid = (*env)->GetMethodID(env, classClass, "getName", "()Ljava/lang/String;");
            exceptionName = (jstring)(*env)->CallObjectMethod(env, exceptionClass, mid);
            exceptionNameUTF8 = (*env)->GetStringUTFChars(env, exceptionName, 0);

            mid = (*env)->GetMethodID(env, exceptionClass, "getMessage", "()Ljava/lang/String;");
            exceptionMessage = (jstring)(*env)->CallObjectMethod(env, exception, mid);

            if (exceptionMessage != nullptr) {
                const char *exceptionMessageUTF8 = (*env)->GetStringUTFChars(env, exceptionMessage, 0);
                _cc_logger_error(_T("%s: %s"), exceptionNameUTF8, exceptionMessageUTF8);
                (*env)->ReleaseStringUTFChars(env, exceptionMessage, exceptionMessageUTF8);
            } else {
                _cc_logger_error(_T("%s"), exceptionNameUTF8);
            }

            (*env)->ReleaseStringUTFChars(env, exceptionName, exceptionNameUTF8);
        }

        return true;
    }

    return false;
}

_CC_API_PRIVATE(void) Internal_Android_Create_AssetManager(void) {
    struct LocalReferenceHolder refs = LocalReferenceHolder_Setup(__FUNCTION__);
    JNIEnv *env = Android_JNI_GetEnv();
    jmethodID mid;
    jobject context;
    jobject javaAssetManager;

    if (!LocalReferenceHolder_Init(&refs, env)) {
        LocalReferenceHolder_Cleanup(&refs);
        return;
    }

    // context = CCWidgets.getContext();
    context = (*env)->CallStaticObjectMethod(env, mWidgetsClass, midGetContext);

    // javaAssetManager = context.getAssets();
    mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getAssets", "()Landroid/content/res/AssetManager;");
    javaAssetManager = (*env)->CallObjectMethod(env, context, mid);

    /**
     * Given a Dalvik AssetManager object, obtain the corresponding native AAssetManager
     * object.  Note that the caller is responsible for obtaining and holding a VM reference
     * to the jobject to prevent its being garbage collected while the native object is
     * in use.
     */
    javaAssetManagerRef = (*env)->NewGlobalRef(env, javaAssetManager);
    asset_manager = AAssetManager_fromJava(env, javaAssetManagerRef);

    if (!asset_manager) {
        (*env)->DeleteGlobalRef(env, javaAssetManagerRef);
        Android_JNI_ExceptionOccurred(true);
    }

    LocalReferenceHolder_Cleanup(&refs);
}

_CC_API_PRIVATE(void) Internal_Android_Destroy_AssetManager(void) {
    JNIEnv *env = Android_JNI_GetEnv();

    if (asset_manager) {
        (*env)->DeleteGlobalRef(env, javaAssetManagerRef);
        asset_manager = nullptr;
    }
}

_CC_API_PUBLIC(bool_t) Android_JNI_FileOpen(pvoid_t *puserdata, const tchar_t *fileName, const char *mode) {
    AAsset *asset = nullptr;

    _cc_assert(puserdata != nullptr);
    *puserdata = nullptr;

    if (!asset_manager) {
        Internal_Android_Create_AssetManager();
    }

    if (!asset_manager) {
        _cc_logger_error(_T("Couldn't create asset manager"));
        return false;
    }

    asset = AAssetManager_open(asset_manager, fileName, AASSET_MODE_UNKNOWN);
    if (!asset) {
        _cc_logger_error(_T("Couldn't open asset %s"), fileName);
        return false;
    }

    *puserdata = (pvoid_t )asset;
    return true;
}

_CC_API_PUBLIC(size_t) Android_JNI_FileRead(pvoid_t userdata, pvoid_t buffer, size_t size) {
    const int bytes = AAsset_read((AAsset *)userdata, buffer, size);
    if (bytes < 0) {
        _cc_logger_error(_T("AAsset_read() failed"));
        return 0;
    }
    return (size_t)bytes;
}

_CC_API_PUBLIC(size_t) Android_JNI_FileWrite(pvoid_t userdata, const pvoid_t buffer, size_t size) {
    _cc_logger_error(_T("Cannot write to Android package filesystem"));
    return 0;
}

_CC_API_PUBLIC(int64_t) Android_JNI_FileSize(pvoid_t userdata) {
    return (int64_t) AAsset_getLength64((AAsset *)userdata);
}

_CC_API_PUBLIC(int64_t) Android_JNI_FileSeek(pvoid_t userdata, int64_t offset, int whence) {
    return (int64_t) AAsset_seek64((AAsset *)userdata, offset, whence);
}

_CC_API_PUBLIC(bool_t) Android_JNI_FileFlush(pvoid_t userdata) {
    return true;
}

_CC_API_PUBLIC(bool_t) Android_JNI_FileClose(pvoid_t userdata) {
    AAsset_close((AAsset *)userdata);
    return true;
}

_CC_API_PUBLIC(bool_t) Android_JNI_SetClipboardText(const tchar_t *buf) {
    JNIEnv *env = Android_JNI_GetEnv();
    jstring string = (*env)->NewStringUTF(env, buf);
    (*env)->CallStaticVoidMethod(env, mWidgetsClass, midClipboardSetText, string);
    (*env)->DeleteLocalRef(env, string);
    return true;
}

_CC_API_PUBLIC(int32_t) Android_JNI_GetClipboardText(tchar_t *buf, size_t buf_length) {
    JNIEnv *env = Android_JNI_GetEnv();
    size_t utf_length = 0;
    jstring string = (*env)->CallStaticObjectMethod(env, mWidgetsClass, midClipboardGetText);
    if (string) {
        const tchar_t *utf = (*env)->GetStringUTFChars(env, string, 0);
        if (utf) {
            utf_length = (*env)->GetStringUTFLength(env, string);
            if (utf_length > buf_length) {
                utf_length = buf_length;
            }
            memcpy(buf, utf, utf_length);
            buf[utf_length] = 0;
            (*env)->ReleaseStringUTFChars(env, string, utf);
        }
        (*env)->DeleteLocalRef(env, string);
    }

    return utf_length;
}

_CC_API_PUBLIC(bool_t) Android_JNI_HasClipboardText(void) {
    JNIEnv *env = Android_JNI_GetEnv();
    return (*env)->CallStaticBooleanMethod(env, mWidgetsClass, midClipboardHasText);
}

/* returns 0 on success or -1 on error (others undefined then)
 * returns truthy or falsy value in plugged, charged and battery
 * returns the value in seconds and percent or -1 if not available
 */
_CC_API_PUBLIC(void) Android_JNI_GetPowerInfo(int *plugged, int *charged, int *battery, int *seconds, byte_t *percent) {
    JNIEnv *env = Android_JNI_GetEnv();

    if (!androidPowerInfo.registered) {
        (*env)->CallStaticVoidMethod(env, mWidgetsClass, midRegisterPowerInfo);
        androidPowerInfo.registered = true;
    }

    if (plugged) {
        *plugged = androidPowerInfo.plugged;
    }

    if (charged) {
        *charged = androidPowerInfo.charged;
    }

    if (battery) {
        *battery = androidPowerInfo.battery;
    }

    if (seconds) {
        *seconds = androidPowerInfo.seconds;
    }

    if (percent) {
        *percent = (byte_t)androidPowerInfo.percent;
    }
}
_CC_API_PUBLIC(int) GetAndroidSDKVersion(void) {
    static int sdk_version;
    if (!sdk_version) {
        char sdk[PROP_VALUE_MAX] = { 0 };
        if (__system_property_get("ro.build.version.sdk", sdk) != 0) {
            sdk_version = atoi(sdk);
        }
    }
    return sdk_version;
}

_CC_API_PUBLIC(bool_t) IsAndroidTablet(void) {
    JNIEnv *env = Android_JNI_GetEnv();
    return (*env)->CallStaticBooleanMethod(env, mWidgetsClass, midIsTablet);
}

_CC_API_PUBLIC(bool_t) IsAndroidTV(void) {
    JNIEnv *env = Android_JNI_GetEnv();
    return (*env)->CallStaticBooleanMethod(env, mWidgetsClass, midIsAndroidTV);
}

_CC_API_PUBLIC(bool_t) IsChromebook(void) {
    JNIEnv *env = Android_JNI_GetEnv();
    return (*env)->CallStaticBooleanMethod(env, mWidgetsClass, midIsChromebook);
}

_CC_API_PUBLIC(bool_t) IsDeXMode(void) {
    JNIEnv *env = Android_JNI_GetEnv();
    return (*env)->CallStaticBooleanMethod(env, mWidgetsClass, midIsDeXMode);
}

_CC_API_PUBLIC(const _cc_String_t *) GetAndroidInternalStoragePath(void) {
    if (s_AndroidInternalFilesPath.length == 0) {
        struct LocalReferenceHolder refs = LocalReferenceHolder_Setup(__FUNCTION__);
        jmethodID mid;
        jobject context;
        jobject fileObject;
        jstring pathString;
        const tchar_t *path;

        JNIEnv *env = Android_JNI_GetEnv();
        if (!LocalReferenceHolder_Init(&refs, env)) {
            LocalReferenceHolder_Cleanup(&refs);
            return nullptr;
        }

        // context = CCWidgets.getContext();
        context = (*env)->CallStaticObjectMethod(env, mWidgetsClass, midGetContext);
        if (!context) {
            _cc_logger_error(_T("Couldn't get Android context!"));
            LocalReferenceHolder_Cleanup(&refs);
            return nullptr;
        }

        // fileObj = context.getFilesDir();
        mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getFilesDir", "()Ljava/io/File;");
        fileObject = (*env)->CallObjectMethod(env, context, mid);
        if (!fileObject) {
            _cc_logger_error(_T("Couldn't get internal directory"));
            LocalReferenceHolder_Cleanup(&refs);
            return nullptr;
        }

        // path = fileObject.getCanonicalPath();
        mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, fileObject), "getCanonicalPath", "()Ljava/lang/String;");
        pathString = (jstring)(*env)->CallObjectMethod(env, fileObject, mid);
        if (Android_JNI_ExceptionOccurred(false)) {
            LocalReferenceHolder_Cleanup(&refs);
            return nullptr;
        }

        path = (*env)->GetStringUTFChars(env, pathString, nullptr);
        s_AndroidInternalFilesPath.length = (*env)->GetStringUTFLength(env, pathString);
        s_AndroidInternalFilesPath.data = _cc_tcsndup(path,s_AndroidInternalFilesPath.length);
        (*env)->ReleaseStringUTFChars(env, pathString, path);

        LocalReferenceHolder_Cleanup(&refs);
    }
    return &s_AndroidInternalFilesPath;
}

_CC_API_PUBLIC(uint32_t) GetAndroidExternalStorageState(void) {
    struct LocalReferenceHolder refs = LocalReferenceHolder_Setup(__FUNCTION__);
    jmethodID mid;
    jclass cls;
    jstring stateString;
    const char *state_string;
    uint32_t stateFlags;

    JNIEnv *env = Android_JNI_GetEnv();
    if (!LocalReferenceHolder_Init(&refs, env)) {
        LocalReferenceHolder_Cleanup(&refs);
        return 0;
    }

    cls = (*env)->FindClass(env, "android/os/Environment");
    mid = (*env)->GetStaticMethodID(env, cls, "getExternalStorageState", "()Ljava/lang/String;");
    stateString = (jstring)(*env)->CallStaticObjectMethod(env, cls, mid);

    state_string = (*env)->GetStringUTFChars(env, stateString, nullptr);

    // Print an info message so people debugging know the storage state
    __android_log_print(ANDROID_LOG_INFO, _CC_ANDROID_TAG_, "external storage state: %s", state_string);

    if (strcmp(state_string, "mounted") == 0) {
        stateFlags = CC_ANDROID_EXTERNAL_STORAGE_READ |
                     CC_ANDROID_EXTERNAL_STORAGE_WRITE;
    } else if (strcmp(state_string, "mounted_ro") == 0) {
        stateFlags = CC_ANDROID_EXTERNAL_STORAGE_READ;
    } else {
        stateFlags = 0;
    }
    (*env)->ReleaseStringUTFChars(env, stateString, state_string);

    LocalReferenceHolder_Cleanup(&refs);

    return stateFlags;
}

_CC_API_PUBLIC(const _cc_String_t *) GetAndroidExternalStoragePath(void) {

    if (s_AndroidExternalFilesPath.length == 0) {
        struct LocalReferenceHolder refs = LocalReferenceHolder_Setup(__FUNCTION__);
        jmethodID mid;
        jobject context;
        jobject fileObject;
        jstring pathString;
        const tchar_t *path;

        JNIEnv *env = Android_JNI_GetEnv();
        if (!LocalReferenceHolder_Init(&refs, env)) {
            LocalReferenceHolder_Cleanup(&refs);
            return nullptr;
        }

        // context = CCWidgets.getContext();
        context = (*env)->CallStaticObjectMethod(env, mWidgetsClass, midGetContext);

        // fileObj = context.getExternalFilesDir();
        mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");
        fileObject = (*env)->CallObjectMethod(env, context, mid, nullptr);
        if (!fileObject) {
            _cc_logger_error(_T("Couldn't get external directory"));
            LocalReferenceHolder_Cleanup(&refs);
            return nullptr;
        }

        // path = fileObject.getAbsolutePath();
        mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, fileObject), "getAbsolutePath", "()Ljava/lang/String;");
        pathString = (jstring)(*env)->CallObjectMethod(env, fileObject, mid);

        path = (*env)->GetStringUTFChars(env, pathString, nullptr);
        s_AndroidExternalFilesPath.length = (*env)->GetStringUTFLength(env, pathString);
        s_AndroidExternalFilesPath.data = _cc_tcsndup(path,s_AndroidExternalFilesPath.length);
        (*env)->ReleaseStringUTFChars(env, pathString, path);

        LocalReferenceHolder_Cleanup(&refs);
    }
    return &s_AndroidExternalFilesPath;
}

_CC_API_PUBLIC(const _cc_String_t *) GetAndroidCachePath(void) {
    // !!! FIXME: lots of duplication with GetAndroidExternalStoragePath and GetAndroidInternalStoragePath; consolidate these functions!
    if (s_AndroidCachePath.length == 0) {
        struct LocalReferenceHolder refs = LocalReferenceHolder_Setup(__FUNCTION__);
        jmethodID mid;
        jobject context;
        jobject fileObject;
        jstring pathString;
        const tchar_t *path;

        JNIEnv *env = Android_JNI_GetEnv();
        if (!LocalReferenceHolder_Init(&refs, env)) {
            LocalReferenceHolder_Cleanup(&refs);
            return nullptr;
        }

        // context = CCWidgets.getContext();
        context = (*env)->CallStaticObjectMethod(env, mWidgetsClass, midGetContext);

        // fileObj = context.getExternalFilesDir();
        mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getCacheDir", "()Ljava/io/File;");
        fileObject = (*env)->CallObjectMethod(env, context, mid, nullptr);
        if (!fileObject) {
            _cc_logger_error(_T("Couldn't get cache directory"));
            LocalReferenceHolder_Cleanup(&refs);
            return nullptr;
        }

        // path = fileObject.getAbsolutePath();
        mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, fileObject),  "getAbsolutePath", "()Ljava/lang/String;");
        pathString = (jstring)(*env)->CallObjectMethod(env, fileObject, mid);

        path = (*env)->GetStringUTFChars(env, pathString, nullptr);
        s_AndroidCachePath.length = (*env)->GetStringUTFLength(env, pathString);
        s_AndroidCachePath.data = _cc_tcsndup(path,s_AndroidCachePath.length);
        (*env)->ReleaseStringUTFChars(env, pathString, path);

        LocalReferenceHolder_Cleanup(&refs);
    }
    return s_AndroidCachePath;
}

_CC_API_PUBLIC(bool_t) ShowAndroidToast(const tchar_t *message, int duration, int gravity, int xOffset, int yOffset) {
    return Android_JNI_ShowToast(message, duration, gravity, xOffset, yOffset);
}

_CC_API_PUBLIC(void) Android_JNI_GetManifestEnvironmentVariables(void) {
    if (!mWidgetsClass || !midGetManifestEnvironmentVariables) {
        __android_log_print(ANDROID_LOG_WARN, _CC_ANDROID_TAG_, "Request to get environment variables before JNI is ready");
        return;
    }

    if (!bHasEnvironmentVariables) {
        JNIEnv *env = Android_JNI_GetEnv();
        bool_t ret = (*env)->CallStaticBooleanMethod(env, mWidgetsClass, midGetManifestEnvironmentVariables);
        if (ret) {
            bHasEnvironmentVariables = true;
        }
    }
}

typedef struct NativePermissionRequestInfo {
    int request_code;
    tchar_t *permission;
    RequestAndroidPermissionCallback_t callback;
    pvoid_t userdata;
    struct NativePermissionRequestInfo *next;
} NativePermissionRequestInfo_t;

static NativePermissionRequestInfo_t pending_permissions;

JNIEXPORT void JNICALL CC_JAVA_INTERFACE(nativePermissionResult)(
    JNIEnv *env, jclass cls,
    jint requestCode, jboolean result) {

    _cc_mutex_lock(Android_ActivityMutex);
    
    NativePermissionRequestInfo_t *prev = &pending_permissions;

    for (NativePermissionRequestInfo_t *info = prev->next; info != nullptr; info = info->next) {
        if (info->request_code == (int) requestCode) {
            prev->next = info->next;
            info->callback(info->userdata, info->permission, result ? true : false);
            _cc_free(info->permission);
            _cc_free(info);
            break;
        }
        prev = info;
    }

    _cc_mutex_unlock(Android_ActivityMutex);
}

_CC_API_PUBLIC(bool_t) Android_JNI_RequestPermission(const tchar_t *permission, RequestAndroidPermissionCallback_t cb, pvoid_t userdata){
    if (!permission) {
        _cc_logger_error(_T("Parameter permission is invalid"));
        return false;
    } else if (!cb) {
        _cc_logger_error(_T("Parameter cb is invalid"));
        return false;
    }

    NativePermissionRequestInfo_t *info = (NativePermissionRequestInfo_t *) _cc_calloc(1, sizeof (NativePermissionRequestInfo_t));
    if (!info) {
        return false;
    }

    info->permission = _tcsdup(permission);
    if (!info->permission) {
        _cc_free(info);
        return false;
    }

    static _cc_atomic32_t next_request_code;
    info->request_code = _cc_atomic32_add(&next_request_code, 1);

    info->callback = cb;
    info->userdata = userdata;

    _cc_mutex_lock(Android_ActivityMutex);
    info->next = pending_permissions.next;
    pending_permissions.next = info;
    _cc_mutex_unlock(Android_ActivityMutex);

    JNIEnv *env = Android_JNI_GetEnv();
    jstring jpermission = (*env)->NewStringUTF(env, permission);
    (*env)->CallStaticVoidMethod(env, mWidgetsClass, midRequestPermission, jpermission, info->request_code);
    (*env)->DeleteLocalRef(env, jpermission);

    return true;
}

// Show toast notification
_CC_API_PUBLIC(bool_t) Android_JNI_ShowToast(const tchar_t *message, int duration, int gravity, int xOffset, int yOffset) {
    bool_t result;
    JNIEnv *env = Android_JNI_GetEnv();
    jstring jmessage = (*env)->NewStringUTF(env, message);
    result = (*env)->CallStaticBooleanMethod(env, mWidgetsClass, midShowToast, jmessage, duration, gravity, xOffset, yOffset);
    (*env)->DeleteLocalRef(env, jmessage);
    return result;
}

_CC_API_PUBLIC(bool_t) Android_JNI_GetLocale(tchar_t *buf, size_t buf_length) {
    bool_t result = false;
    if (buf && buf_length > 0) {
        *buf = '\0';
        JNIEnv *env = Android_JNI_GetEnv();
        jstring string = (jstring)(*env)->CallStaticObjectMethod(env, mWidgetsClass, midGetPreferredLocales);
        if (string) {
            const char *utf = (*env)->GetStringUTFChars(env, string, nullptr);
            if (utf) {
                size_t utf_length = (*env)->GetStringUTFLength(env, string);
                if (utf_length > buf_length) {
                    utf_length = buf_length;
                }
                memcpy(buf, utf, utf_length);
                buf[utf_length] = 0;
                (*env)->ReleaseStringUTFChars(env, string, utf);
                result = true;
            }
            (*env)->DeleteLocalRef(env, string);
        }
    }
    return result;
}

_CC_API_PUBLIC(bool_t) Android_JNI_OpenURL(const tchar_t *url) {
    bool_t result;
    JNIEnv *env = Android_JNI_GetEnv();
    jstring jurl = (*env)->NewStringUTF(env, url);
    result = (*env)->CallStaticBooleanMethod(env, mWidgetsClass, midOpenURL, jurl);
    (*env)->DeleteLocalRef(env, jurl);
    return result;
}

_CC_API_PUBLIC(int) Android_JNI_OpenFileDescriptor(const tchar_t *uri, const tchar_t *mode) {
    // Get fopen-style modes
    const tchar_t *p;
    const char *modeContentResolver = "r";

    int r = 0, w = 0, a = 0, u = 0;

    for (p = mode; *p; p++) {
        switch (*p) {
            case 'a':
                a = 1;
                break;
            case 'r':
                r = 1;
                break;
            case 'w':
                w = 1;
                break;
            case '+':
                u = 1;
                break;
            default:
                break;
        }
    }

    // Translate fopen-style modes to ContentResolver modes.
    // Android only allows "r", "w", "wt", "wa", "rw" or "rwt".
    if (r) {
        if (w) {
            modeContentResolver = "rwt";
        } else {
            modeContentResolver = u ? "rw" : "r";
        }
    } else if (w) {
        modeContentResolver = u ? "rwt" : "wt";
    } else if (a) {
        modeContentResolver = u ? "rw" : "wa";
    }

    JNIEnv *env = Android_JNI_GetEnv();
    jstring jstringUri = (*env)->NewStringUTF(env, uri);
    jstring jstringMode = (*env)->NewStringUTF(env, modeContentResolver);
    jint fd = (*env)->CallStaticIntMethod(env, mWidgetsClass, midOpenFileDescriptor, jstringUri, jstringMode);
    (*env)->DeleteLocalRef(env, jstringUri);
    (*env)->DeleteLocalRef(env, jstringMode);

    // if (fd == -1) {
    //     _cc_logger_error("Unspecified error in JNI");
    // }

    return fd;
}