#include <stdio.h>
#include <libcc.h>

/* Library init */
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv* env = Android_JNI_OnLoad(vm, reserved, JNI_VERSION_1_6);
    return JNI_VERSION_1_6;
}

void JNI_OnUnload(JavaVM *vm, void *reserved) {

}

_CC_API_PRIVATE(jobject) getApplication(JNIEnv *env) {
    jclass localClass = (*env)->FindClass(env, "android/app/ActivityThread");
    if (localClass != nullptr) {
        jmethodID mApplication = (*env)->GetStaticMethodID(env, localClass, "currentApplication","()Landroid/app/Application;");
        if (mApplication != nullptr) {
            return (*env)->CallStaticObjectMethod(env, localClass, mApplication);
        }
    }
    return nullptr;
}

_CC_API_PRIVATE(void) dumpSign() {
    JNIEnv *env = Android_JNI_GetEnv();
    int length;
    jbyte *bytes;
    jobjectArray signatures;
    jbyteArray signatureBytes;
    jclass activity,packageManagerClass,packageInfoClass,signatureClass;
    jmethodID mid,mPackageName,mPackageInfo,mByteArray;
    jfieldID signaturesField;
    jstring packageName;
    jobject packageManager, packageInfo, signature;
    jobject context = getApplication(env);

    if (context == nullptr) {
        return ;
    }

    activity = (*env)->GetObjectClass(env, context);
    mid = (*env)->GetMethodID(env, activity, "getPackageManager", "()Landroid/content/pm/PackageManager;");
    packageManager = (*env)->CallObjectMethod(env, context, mid);
    packageManagerClass = (*env)->GetObjectClass(env, packageManager);
    mPackageName = (*env)->GetMethodID(env, activity, "getPackageName", "()Ljava/lang/String;");
    packageName = (jstring)((*env)->CallObjectMethod(env, context, mPackageName));
    mPackageInfo = (*env)->GetMethodID(env, packageManagerClass, "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
    packageInfo = (*env)->CallObjectMethod(env, packageManager, mPackageInfo, packageName, 64);
    packageInfoClass = (*env)->GetObjectClass(env, packageInfo);
    signaturesField = (*env)->GetFieldID(env, packageInfoClass, "signatures", "[Landroid/content/pm/Signature;");
    signatures = (jobjectArray)((*env)->GetObjectField(env, packageInfo, signaturesField));
    signature = (*env)->GetObjectArrayElement(env, signatures, 0);
    signatureClass = (*env)->GetObjectClass(env, signature);
    mByteArray = (*env)->GetMethodID(env, signatureClass, "toByteArray", "()[B");
    signatureBytes = (jbyteArray)(*env)->CallObjectMethod(env, signature, mByteArray);


    // 对获取的key进程16进制转换
    length = (*env)->GetArrayLength(env, signatureBytes);
    bytes = (*env)->GetByteArrayElements(env, signatureBytes, 0);
    {
        tchar_t results[33];
        _cc_md5((byte_t*)bytes, length, results);
        _cc_logger_debug("Signature:%s", results);
    }
}