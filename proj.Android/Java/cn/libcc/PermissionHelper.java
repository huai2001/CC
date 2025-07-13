package cn.libcc;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;

import java.util.ArrayList;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

public class PermissionHelper {
    public static void requestPermissions(Activity activity, String permissions[], int requestCode) {
        if (Build.VERSION.SDK_INT < 23 /* Android 6.0 (M) */) {
            CCWidgets.nativePermissionResult(requestCode, true);
            return;
        }

        ArrayList<String> mPermissionList = new ArrayList<>();
        for (int i = 0; i < permissions.length; i++) {
            if (activity.checkSelfPermission(permissions[i]) != PackageManager.PERMISSION_GRANTED) {
                mPermissionList.add(permissions[i]);
            } else {
                CCWidgets.nativePermissionResult(requestCode, true);
            }
        }

        if (!mPermissionList.isEmpty()) {
            permissions = mPermissionList.toArray(new String[mPermissionList.size()]);
            activity.requestPermissions(permissions, requestCode);
        }
    }

    public static boolean hasPermission(Context context, String permission) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            int result = context.checkSelfPermission(permission);
            return PackageManager.PERMISSION_GRANTED == result;
        }

        return true;
    }

    public static void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        for (int i = 0; i < grantResults.length; i++) {
            boolean result = (grantResults[i] == PackageManager.PERMISSION_GRANTED);
            CCWidgets.nativePermissionResult(requestCode, result);
        }
    }
}
