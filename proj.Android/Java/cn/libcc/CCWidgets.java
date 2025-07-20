package cn.libcc;

import java.io.File;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;
import android.telephony.TelephonyManager;

public class CCWidgets  {

    public static native String nativeGetVersion();
    public static native boolean nativeSetupJNI();

    public static native void nativeQuit();
    public static native void nativePause();
    public static native void nativeResume();
    public static native void nativePermissionResult(int requestCode, boolean result);
    //public static native String nativeGetHint(String name);
    //public static native boolean nativeGetHintBoolean(String name, boolean default_value);
    public static native void nativeSetPowerInfo(int plugged,int charged,int battery,int seconds,int percent);

    public static native void nativeDropFile(String filename);
    public static native void nativeClipboardChanged();
    public static native void nativeLocaleChanged();
    public static native void nativeDarkModeChanged(boolean enabled);

    public static native void nativeFocusChanged(boolean hasFocus);

    public static boolean mHasFocus;

    public static final int NetworkType_NotPermission = 0;
    public static final int NetworkType_None = 0;
    public static final int NetworkType_WIFI = 1;
    public static final int NetworkType_2G = 2;
    public static final int NetworkType_3G = 3;
    public static final int NetworkType_4G = 4
    public static final int NetworkType_5G = 5;
    public static final int NetworkType_5GA = 6;

    protected static Locale mCurrentLocale;
    protected static IClipboardHandler iClipboardHandler = null ;
    public static String TAG = "CC-JNI";
    public static Context context = null;

    static {
        Loader.staticInit();
    }

    public static void initialize(Context context) {
        nativeSetupJNI();

        CCWidgets.context = context;
        iClipboardHandler = new ClipboardHandler();
        mHasFocus = true;

        try {
            if (Build.VERSION.SDK_INT < 24 /* Android 7.0 (N) */) {
                mCurrentLocale = getContext().getResources().getConfiguration().locale;
            } else {
                mCurrentLocale = getContext().getResources().getConfiguration().getLocales().get(0);
            }
        } catch(Exception ignored) {
        }

        switch (getContext().getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK) {
        case Configuration.UI_MODE_NIGHT_NO:
            nativeDarkModeChanged(false);
            break;
        case Configuration.UI_MODE_NIGHT_YES:
            nativeDarkModeChanged(true);
            break;
        }
    }

    public static void onConfigurationChanged(Configuration newConfig) {
        Log.v(TAG, "onConfigurationChanged()");

        if (mCurrentLocale == null || !mCurrentLocale.equals(newConfig.locale)) {
            mCurrentLocale = newConfig.locale;
            CCWidgets.nativeLocaleChanged();
        }

        switch (newConfig.uiMode & Configuration.UI_MODE_NIGHT_MASK) {
        case Configuration.UI_MODE_NIGHT_NO:
            CCWidgets.nativeDarkModeChanged(false);
            break;
        case Configuration.UI_MODE_NIGHT_YES:
            CCWidgets.nativeDarkModeChanged(true);
            break;
        }
    }

    public static void onWindowFocusChanged(boolean hasFocus) {
        //Log.v(TAG, "onWindowFocusChanged(): " + hasFocus);
        mHasFocus = hasFocus;
        if (hasFocus) {
           nativeFocusChanged(true);
        } else {
           nativeFocusChanged(false);
        }
    }
    /**
     * Get the device's Universally Unique Identifier (UUID).
     */
    public static String getUuid(Context context) {
        return Settings.Secure.getString(context.getContentResolver(), android.provider.Settings.Secure.ANDROID_ID);
    }

    /**
     * 获取手机序列号
     */
    @SuppressLint({"NewApi", "MissingPermission"})
    public static String getSerialNumber() {
        String serial = "";
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {//9.0+
                serial = Build.getSerial();
            } else if (Build.VERSION.SDK_INT > Build.VERSION_CODES.N) {//8.0+
                serial = Build.SERIAL;
            } else {//8.0-
                Class<?> c = Class.forName("android.os.SystemProperties");
                Method get = c.getMethod("get", String.class);
                serial = (String) get.invoke(c, "ro.serialno");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return serial;
    }

    public static String getModel() {
        return android.os.Build.MODEL;
    }

    public static String getProductName() {
        return android.os.Build.PRODUCT;
    }

    public static String getManufacturer() {
        return android.os.Build.MANUFACTURER;
    }

    /**
     * Get the OS version.
     */
    public static String getOSVersion() {
        return android.os.Build.VERSION.RELEASE;
    }

    public static String getSDKVersion() {
        return android.os.Build.VERSION.SDK;
    }

    /**
     * This method is called by CC using JNI.
     */
    public static Context getContext() {
        return context;
    }
    /**
     * This method is called by CC using JNI.
     */
    public static boolean isSimulator() {
        return android.os.Build.FINGERPRINT.contains("generic") || android.os.Build.PRODUCT.contains("sdk");
    }
    /**
     * This method is called by CC using JNI.
     */
    public static void registerPowerInfo() {
        context.registerReceiver(new BatteryBroadcastReceiver(), new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
    }
   /**
     * This method is called by CC using JNI.
     */
    public static void requestPermission(String permission, int requestCode) {
        PermissionHelper.requestPermissions((Activity)getContext(), new String[]{permission}, requestCode);
    }
    /**
     * This method is called by CC using JNI.
     */
    public static boolean clipboardHasText() {
        return iClipboardHandler.clipboardHasText();
    }

    /**
     * This method is called by CC using JNI.
     */
    public static String clipboardGetText() {
        return iClipboardHandler.clipboardGetText();
    }

    /**
     * This method is called by CC using JNI.
     */
    public static void clipboardSetText(String string) { iClipboardHandler.clipboardSetText(string); }

    /**
     * This method is called by CC using JNI.
     */
    public static int openFileDescriptor(String uri, String mode) throws Exception {
        if (CCWidgets.context == null) {
            return -1;
        }

        try {
            ParcelFileDescriptor pfd = CCWidgets.context.getContentResolver().openFileDescriptor(Uri.parse(uri), mode);
            return pfd != null ? pfd.detachFd() : -1;
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return -1;
        }
    }

    /**
     * This method is called by CC using JNI.
     */
    public static String getCachePath() {
        return context.getCacheDir().getPath();
    }
    /**
     * This method is called by CC using JNI.
     */
    public static File getFilesDir() {
        for (int a = 0; a < 10; a++) {
            File path = context.getFilesDir();
            if (path != null) {
                return path;
            }
        }
        try {
            ApplicationInfo info = context.getApplicationInfo();
            File path = new File(info.dataDir, "files");
            path.mkdirs();
            return path;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    public static String getFilesPath() {
        return getFilesDir().getAbsolutePath();
    }

    /**
     * This method is called by CC using JNI.
     */
    public static String getPackageName() {
        return context.getPackageName();
    }

    public static Boolean isWifiConnected() {
        return Boolean.valueOf(getNetworkType() == NetworkType_WIFI);
    }

    public static Boolean isBluetoothEnabled() {
        BluetoothAdapter defaultAdapter;
        try {
            if (context.getPackageManager().checkPermission("android.permission.BLUETOOTH", context.getPackageName()) != 0 ||
               (defaultAdapter = BluetoothAdapter.getDefaultAdapter()) == null) {
                return false;
            }
            return Boolean.valueOf(defaultAdapter.isEnabled());
        } catch (Exception unused) {
            return false;
        }
    }

    public static int getNetworkType() {
        if (context.checkCallingOrSelfPermission("android.permission.ACCESS_NETWORK_STATE") != 0) {
            return NetworkType_NotPermission;
        }

        ConnectivityManager connectivity = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);

        if (connectivity == NULL){
            return NetworkType_None;
        }

        NetworkInfo networkInfo = connectivity.getActiveNetworkInfo();
        if (networkInfo != null && networkInfo.isConnected()) {
            if (networkInfo.getType() == ConnectivityManager.TYPE_WIFI) {
                return NetworkType_WIFI;
            } else if (networkInfo.getType() == ConnectivityManager.TYPE_MOBILE) {
                String _strSubTypeName = networkInfo.getSubtypeName();
                // TD-SCDMA   networkType is 17
                int networkType = networkInfo.getSubtype();
                switch (networkType) {
                    case TelephonyManager.NETWORK_TYPE_GPRS:
                    case TelephonyManager.NETWORK_TYPE_EDGE:
                    case TelephonyManager.NETWORK_TYPE_CDMA:
                    case TelephonyManager.NETWORK_TYPE_1xRTT:
                    case TelephonyManager.NETWORK_TYPE_IDEN: //api<8 : replace by 11
                        return NetworkType_2G;
                    case TelephonyManager.NETWORK_TYPE_UMTS:
                    case TelephonyManager.NETWORK_TYPE_EVDO_0:
                    case TelephonyManager.NETWORK_TYPE_EVDO_A:
                    case TelephonyManager.NETWORK_TYPE_HSDPA:
                    case TelephonyManager.NETWORK_TYPE_HSUPA:
                    case TelephonyManager.NETWORK_TYPE_HSPA:
                    case TelephonyManager.NETWORK_TYPE_EVDO_B: //api<9 : replace by 14
                    case TelephonyManager.NETWORK_TYPE_EHRPD:  //api<11 : replace by 12
                    case TelephonyManager.NETWORK_TYPE_HSPAP:  //api<13 : replace by 15
                        return NetworkType_3G;
                    case TelephonyManager.NETWORK_TYPE_LTE:    //api<11 : replace by 13
                        return NetworkType_4G;
                    default:
                        // http://baike.baidu.com/item/TD-SCDMA 中国移动 联通 电信 三种3G制式
                        if (_strSubTypeName.equalsIgnoreCase("TD-SCDMA") ||
                                _strSubTypeName.equalsIgnoreCase("WCDMA") ||
                                _strSubTypeName.equalsIgnoreCase("CDMA2000")) {
                            return NetworkType_3G;
                        }

                        return NetworkType_WIFI;
                }
            }
        }
        return NetworkType_None;
    }

    /**
     * This method is called by CC using JNI.
     */
    public static int openURL(String url) {
        try {
            Intent i = new Intent(Intent.ACTION_VIEW);
            i.setData(Uri.parse(url));

            int flags = Intent.FLAG_ACTIVITY_NO_HISTORY | Intent.FLAG_ACTIVITY_MULTIPLE_TASK;
            if (Build.VERSION.SDK_INT >= 26) {
                flags |= Intent.FLAG_ACTIVITY_NEW_DOCUMENT;
            } else {
                flags |= Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET;
            }
            i.addFlags(flags);

            context.startActivity(i);
        } catch (Exception ex) {
            return -1;
        }
        return 0;
    }

    /**
     * This method is called by libcc using JNI.
     */
    public static String getPreferredLocales() {
        String result = "";
        if (Build.VERSION.SDK_INT >= 24 /* Android 7 (N) */) {
            LocaleList locales = LocaleList.getAdjustedDefault();
            for (int i = 0; i < locales.size(); i++) {
                if (i != 0) result += ",";
                result += formatLocale(locales.get(i));
            }
        } else if (mCurrentLocale != null) {
            result = formatLocale(mCurrentLocale);
        }
        return result;
    }

    public static String formatLocale(Locale locale) {
        String result = "";
        String lang = "";
        if (locale.getLanguage() == "in") {
            // Indonesian is "id" according to ISO 639.2, but on Android is "in" because of Java backwards compatibility
            lang = "id";
        } else if (locale.getLanguage() == "") {
            // Make sure language is never empty
            lang = "und";
        } else {
            lang = locale.getLanguage();
        }

        if (locale.getCountry() == "") {
            result = lang;
        } else {
            result = lang + "_" + locale.getCountry();
        }
        return result;
    }

    public static void showToast2(String msg) {
        Utils.toast(CCWidgets.context, msg);
    } 
    /**
     * This method is called by CC using JNI.
     */
    public static void showToast(String m, int duration, int gravity, int xOffset, int yOffset)  {
        Utils.toast(context, m, duration, gravity, xOffset, yOffset);
    }
}
