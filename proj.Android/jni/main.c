#include <stdio.h>
#include <libcc.h>

#define _CC_JNI_CONCAT(class, function)       Java_com_libcc_qiu_ ## class ## _ ## function
#define _CC_JNI_INTERFACE(function)           _CC_JNI_CONCAT(MainActivity, function)

static JNIEnv *_JNIEnv = NULL;
/* Main activity */
static jclass _activity_class = 0;

/* Java class */
JNIEXPORT jboolean JNICALL _CC_JNI_INTERFACE(nativeSetupJNI)(JNIEnv *env,
                                                             jclass jcls);

JNIEXPORT jint JNICALL _CC_JNI_INTERFACE(traverseDirectory)(JNIEnv *env,
                                                           jclass jcls,
                                                           jstring save,
                                                           jstring path);
static JNINativeMethod _cc_jni_native_methods[] = {
    { "nativeSetupJNI",   "()Z", _CC_JNI_INTERFACE(nativeSetupJNI) },
    { "traverseDirectory", "(Ljava/lang/String;Ljava/lang/String;)Z", _CC_JNI_INTERFACE(traverseDirectory) },
};


//
#define FILTER_FILE 0
#define FILTER_EXTENSION 1

#define CHUNK 16384
int32_t removeDirectoryLen = 0;

#if FILTER_FILE
/**/
const tchar_t* filterFileList[] = {
#if defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    _T(".DS_Store"),
#endif
    _T("Builder"),
    _T(".bash_profile"),
    _T(".bashrc"),
    _T(".bash_logout"),
    _T(".svn"),
};
#endif

static bool_t isFillerList(tchar_t *name, int32_t namlen) {
    int32_t i = 0;
    if ((name[0]=='.' && name[1] == 0) ||
        (name[0]=='.' && name[1] == '.' && name[2] == 0)) {
        return true;
    }

#if FILTER_FILE
    for (i = 0; i < _cc_countof(filterFileList); i++) {
        if (_tcsnicmp(filterFileList[i], name, namlen) == 0) {
            return true;
        }
    }
#endif
    return false;
}

bool_t OpenDeepDirectory(const tchar_t *directory, _cc_file_t *file) {
    tchar_t sourceFile[_CC_MAX_PATH_] = {0};
    tchar_t results[33];
    int32_t len;
    DIR *dpath;
    struct dirent *d;
    struct _stat stat_buf;
    
    if( (dpath = opendir(directory)) == NULL) {
        _cc_logger_error(_T("Couldn't open directory:%s"), directory);
        return false;
    }
    
    while ((d = readdir(dpath)) != NULL) {
        if (isFillerList(d->d_name, d->d_reclen)) continue;

        sourceFile[0] = 0;
        len = _sntprintf(sourceFile, _cc_countof(sourceFile), "%s/%s", directory, d->d_name);
        _tstat( sourceFile, &stat_buf);

        if (S_ISDIR(stat_buf.st_mode) == 0) {
            _cc_file_write(file, sourceFile, len, sizeof(char_t));
            _cc_md5file(sourceFile, results);
            _cc_file_write(file, ",", 1, sizeof(char_t));
            _cc_file_write(file, results, 32, sizeof(char_t));
            _cc_file_write(file, "\n", 1, sizeof(char_t));
        } else {
            OpenDeepDirectory(sourceFile, file);
        }
    }
    closedir(dpath);
}
//

JNIEXPORT jint JNICALL _CC_JNI_INTERFACE(traverseDirectory)(JNIEnv *env,
                                                           jclass cls,
                                                           jstring save,
                                                           jstring path) {

    _cc_file_t *w;
    const char *csave = (*env)->GetStringUTFChars(env, save, NULL);
    const char *cpath = (*env)->GetStringUTFChars(env, path, NULL);

    w = _cc_open_file(csave, "w");
    if (w) {
        OpenDeepDirectory(cpath, w);
        _cc_file_close(w);
    }

    //_cc_logger_debug("done");

    (*env)->ReleaseStringUTFChars(env, save, csave);
    (*env)->ReleaseStringUTFChars(env, path, cpath);
    return 0;
}

_CC_FORCE_INLINE_ void register_methods(JNIEnv *env,
                                        const char *classname,
                                        JNINativeMethod *methods,
                                        int nb)
{
    jclass clazz = (*env)->FindClass(env, classname);
    if (clazz == NULL || (*env)->RegisterNatives(env, clazz, methods, nb) < 0) {
        _cc_logger_error("Failed to register methods of %s", classname);
        return;
    }
}

#ifdef _CC_JNI_BUILD_SHARED_LIBRARY_
/* Library init */
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    
    JNIEnv *env = _cc_jni_onload(vm, reserved, JNI_VERSION_1_4);
    
    register_methods(env, "com/libcc/qiu/MainActivity", _cc_jni_native_methods, _cc_countof(_cc_jni_native_methods));
    return JNI_VERSION_1_4;
}
#endif

/* Activity initialization */
JNIEXPORT jboolean JNICALL _CC_JNI_INTERFACE(nativeSetupJNI)(JNIEnv *env,
                                                             jclass cls) {
    _cc_logger_debug("nativeSetupJNI2()");
    _JNIEnv = env;
    _activity_class = (jclass)((*env)->NewGlobalRef(env, cls));


    return JNI_TRUE;
}