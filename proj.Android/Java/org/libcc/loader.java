package org.libcc;

import android.content.pm.ApplicationInfo;
import android.os.Process;

import java.io.File;
import java.lang.reflect.Field;

public class loader {
    private static final String LIB_LOADER_NAME = "cc";
    private static final String TAG = "runtime.Setup";
    private static boolean initialized = false;
    private static String sandboxPath;

    public static Object getFieldValue(String str, Object obj, String str2) {
        try {
            Field declaredField = Class.forName(str).getDeclaredField(str2);
            declaredField.setAccessible(true);
            return declaredField.get(obj);
        } catch (NoSuchFieldException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e2) {
            e2.printStackTrace();
        } catch (IllegalArgumentException e3) {
            e3.printStackTrace();
        } catch (ClassNotFoundException e4) {
            e4.printStackTrace();
        }
        return null;
    }

    public static String getSandboxPath() {
        try {
            return new File(((ApplicationInfo)
                    getFieldValue("android.app.LoadedApk", getFieldValue("android.app.ActivityThread$AppBindData", getFieldValue("android.app.ActivityThread", Class.forName("android.app.ActivityThread").getMethod("currentActivityThread", new Class[0]).invoke(null, new Object[0]), "mBoundApplication"), "info"), "mApplicationInfo")).dataDir).getParent() + "/";

        } catch (Exception e) {
            return null;
        }
    }

    public static void staticInit() {
        if (!initialized) {
            initialized = true;
            sandboxPath = getSandboxPath();
            loadLibraries();
            //NativeBridge.setDeviceSerialNo(Build.SERIAL);
            CCWidgets.nativeSetupJNI();
        }
    }

    public static void loadLibraries() {
        try {
            System.loadLibrary(LIB_LOADER_NAME);
        } catch (Exception e) {
            Process.killProcess(Process.myPid());
        } catch (UnsatisfiedLinkError e2) {
            Process.killProcess(Process.myPid());
        }
    }
}
