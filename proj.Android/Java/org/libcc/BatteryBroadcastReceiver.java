package org.libcc;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.BatteryManager;

class BatteryBroadcastReceiver extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent.getAction().equals(Intent.ACTION_BATTERY_CHANGED)) {

            int level = intent.getIntExtra(BatteryManager.EXTRA_LEVEL, 0);
            int scale = intent.getIntExtra(BatteryManager.EXTRA_SCALE, 100);

            int status = intent.getIntExtra(BatteryManager.EXTRA_STATUS, 0);
            int plugged = intent.getIntExtra(BatteryManager.EXTRA_PLUGGED, 0);
            int battery = intent.getIntExtra(BatteryManager.EXTRA_PRESENT, 0);
            int charged = BatteryManager.BATTERY_STATUS_FULL == status ? 1 : 0;

            CCWidgets.setPowerInfo(plugged,charged,battery,-1,(level * 100 / scale));
        }
    }
}