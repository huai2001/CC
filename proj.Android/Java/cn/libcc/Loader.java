package cn.libcc;

import android.content.pm.ApplicationInfo;
import android.os.Process;

import java.io.File;
import java.lang.reflect.Field;

public class Loader {
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
        if (!initialized && Process.myPid() == Process.myTid()) {
            initialized = true;
            sandboxPath = getSandboxPath();
            NativeBridge.setDeviceSerialNo(Build.SERIAL);
        }
    }

    static void loadLibrary(String libraryName) throws UnsatisfiedLinkError, SecurityException, nullPointerException {
        loadLibrary(libraryName, CCWidgets.context);
    }

    static void loadLibrary(String libraryName, Context context) throws UnsatisfiedLinkError, SecurityException, nullPointerException {
        if (libraryName == null) {
            throw new nullPointerException("No library name provided.");
        }

        try {
            // Let's see if we have ReLinker available in the project.  This is necessary for
            // some projects that have huge numbers of local libraries bundled, and thus may
            // trip a bug in Android's native library loader which ReLinker works around.  (If
            // loadLibrary works properly, ReLinker will simply use the normal Android method
            // internally.)
            //
            // To use ReLinker, just add it as a dependency.  For more information, see
            // https://github.com/KeepSafe/ReLinker for ReLinker's repository.
            //
            Class<?> relinkClass = context.getClassLoader().loadClass("com.getkeepsafe.relinker.ReLinker");
            Class<?> relinkListenerClass = context.getClassLoader().loadClass("com.getkeepsafe.relinker.ReLinker$LoadListener");
            Class<?> contextClass = context.getClassLoader().loadClass("android.content.Context");
            Class<?> stringClass = context.getClassLoader().loadClass("java.lang.String");

            // Get a 'force' instance of the ReLinker, so we can ensure libraries are reinstalled if
            // they've changed during updates.
            Method forceMethod = relinkClass.getDeclaredMethod("force");
            Object relinkInstance = forceMethod.invoke(null);
            Class<?> relinkInstanceClass = relinkInstance.getClass();

            // Actually load the library!
            Method loadMethod = relinkInstanceClass.getDeclaredMethod("loadLibrary", contextClass, stringClass, stringClass, relinkListenerClass);
            loadMethod.invoke(relinkInstance, context, libraryName, null, null);
        }
        catch (final Throwable e) {
            // Fall back
            try {
                System.loadLibrary(libraryName);
            }
            catch (final UnsatisfiedLinkError ule) {
                throw ule;
            }
            catch (final SecurityException se) {
                throw se;
            }
        }
    }
}
