package org.libcc;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;

import java.util.ArrayList;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

public class PermissionHelper {
    public static void requestPermissions(Activity activity, String permissions[], int code) {
        ArrayList<String> mPermissionList = new ArrayList<>();
        for (int i = 0; i < permissions.length; i++) {
            if (ContextCompat.checkSelfPermission(CCWidgets.context, permissions[i]) != PackageManager.PERMISSION_GRANTED) {
                mPermissionList.add(permissions[i]);
            }
        }

        if (!mPermissionList.isEmpty()) {
            permissions = mPermissionList.toArray(new String[mPermissionList.size()]);//将List转为数组
            ActivityCompat.requestPermissions(activity, permissions, code);
        }
    }

    public static boolean hasPermission(Context context, String permission)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            int result = context.checkSelfPermission(permission);
            return PackageManager.PERMISSION_GRANTED == result;
        }

        return true;
    }
}
