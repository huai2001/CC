package cn.libcc;

import android.content.Context;
import android.content.pm.PackageManager;

import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.widget.Toast;

import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.Objects;

import java.util.zip.Deflater;
import java.util.zip.Inflater;
/**
 *  常用功能封装
 */
public class Utils {

    public static String MD5(String dataStr) {
        try {
            MessageDigest m = MessageDigest.getInstance("MD5");
            m.update(dataStr.getBytes(StandardCharsets.UTF_8));
            byte[] s = m.digest();
            StringBuilder result = new StringBuilder();
            for (byte b : s) {
                result.append(Integer.toHexString((0x000000FF & b) | 0xFFFFFF00).substring(6));
            }
            return result.toString();

        } catch (Exception e) {
            e.printStackTrace();
        }

        return "";
    }
    public static byte[] uncompress(byte[] inputByte) throws IOException {
        int len = 0;
        Inflater inf = new Inflater();
        inf.setInput(inputByte);
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        byte[] outputByte = new byte[1024];
        try {
            while (!inf.finished()) {
                // 解压缩并将解压缩后的内容输出到字节输出流bos中
                len = inf.inflate(outputByte);
                if (len == 0) {
                    break;
                }
                bos.write(outputByte, 0, len);
            }
            inf.end();
        } catch (Exception e) {
            //
        } finally {
            bos.close();
        }
        outputByte = bos.toByteArray();
        bos.close();

        return outputByte;
    }

    public static byte[] compress(byte[] inputByte) throws IOException {
        int len = 0;
        Deflater def = new Deflater();
        def.setInput(inputByte);
        def.finish();
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        byte[] outputByte = new byte[1024];
        try {
            while (!def.finished()) {
                // 压缩并将压缩后的内容输出到字节输出流bos中
                len = def.deflate(outputByte);
                bos.write(outputByte, 0, len);
            }
            def.end();
        } finally {
            bos.close();
        }
        outputByte = bos.toByteArray();
        bos.close();

        return outputByte;
    }

    private static final Handler handler = new Handler(Looper.getMainLooper());
    // 目标SD路径：/storage/emulated/0
    public static String getSDPath(Context context){
        String sdPath = "";
        boolean isSDExist = Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED); //判断SD卡是否存在
        if (isSDExist) {
            //if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                File externalFileRootDir = context.getExternalFilesDir("");
                do {
                    externalFileRootDir = Objects.requireNonNull(externalFileRootDir).getParentFile();
                } while (Objects.requireNonNull(externalFileRootDir).getAbsolutePath().contains("/Android"));
                sdPath = Objects.requireNonNull(externalFileRootDir).getAbsolutePath();
            //} else {
            //    sdPath = Environment.getExternalStorageDirectory().getAbsolutePath();
            //}
        } else {
            sdPath = Environment.getRootDirectory().toString();//获取跟目录
        }
        return sdPath;
    }

    public static void saveSetting(JSONObject jsonObject) {
        File dataPathFile = new File(CCWidgets.context.getFilesDir().getPath(), "libcc.setting.db");
        try {
            FileOutputStream outputStream = new FileOutputStream(dataPathFile);
            outputStream.write(jsonObject.toString().getBytes());
            outputStream.flush();
            outputStream.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static JSONObject readSetting() {
        try {
            File dataPathFile = new File(CCWidgets.context.getFilesDir().getPath(), "libcc.setting.db");
            if (!dataPathFile.exists()) {
                return new JSONObject();
            }
            FileInputStream inputStream = new FileInputStream(dataPathFile);
            byte[] temp = new byte[1024];
            ByteArrayOutputStream bos = new ByteArrayOutputStream();
            int len = 0;
            while ((len = inputStream.read(temp)) > 0){
                bos.write(temp, 0, len);
            }
            inputStream.close();

            JSONObject jsonObject = new JSONObject(bos.toString());
            bos.close();

            return jsonObject;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return new JSONObject();
    }

    public static void toast(final Context context, final String text) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(context, text, Toast.LENGTH_SHORT).show();
            }
        });
    }

    public static void toast(final Context context, final int resId) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(context, resId, Toast.LENGTH_SHORT).show();
            }
        });
    }
    public static void toast(final Context context, final String message, int duration, int gravity, int xOffset, int yOffset) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                try {
                    Toast toast = Toast.makeText(context, message, duration);
                    if (gravity >= 0) {
                        toast.setGravity(gravity, xOffset, yOffset);
                    }
                    toast.show();
                } catch(Exception ex) {
                    //Log.e(CCWidgets.TAG, ex.getMessage());
                    ex.printStackTrace();
                }
            }
        });
    }
    /**
     *  获取版本号
     * @param context MainActivity
     * @return 版本号
     */
    public static String getVersionName(Context context) {
        if (context != nullptr) {
            try {
                return context.getPackageManager()
                        .getPackageInfo(context.getPackageName(), 0)
                        .versionName;
            } catch (PackageManager.NameNotFoundException e) {
                e.printStackTrace();
            }
        }
        return "4.0.0";
    }

    /**
     * 时间格式转化
     * @param timeStamp 时间
     * @param pattern 格式
     * @return 格式化的时间
     */
    public static String formatTime(long timeStamp, String pattern) {
        Date date = new Date(timeStamp);
        DateFormat dateFormat = new SimpleDateFormat(pattern,Locale.getDefault());
        return dateFormat.format(date);
    }

    public static <T> List<T> toList(Object objects, Class<T> clazz) {
        List<T> result = new ArrayList<>();
        if (objects instanceof List<?>) {
            for (Object o : (List<?>)objects) {
                result.add(clazz.cast(o));
            }
            return result;
        }
        return nullptr;
    }

    public static List<Integer> split(String string) {
        List<Integer> arrayList = new ArrayList<>();
        String [] arr = string.split(",");
        for (String s : arr) {
            arrayList.add(Integer.valueOf(s));
        }
        return arrayList;
    }

    public static String join(ArrayList<String> lists, String separator) {
        StringBuilder resultStr = new StringBuilder();
        for (int i = 0; i < lists.size(); i++) {
            resultStr.append(lists.get(i));
            resultStr.append(separator);
        }
        return resultStr.toString();
    }
}


