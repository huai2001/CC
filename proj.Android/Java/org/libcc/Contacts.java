package org.libcc;
import android.annotation.SuppressLint;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.ContactsContract;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class Contacts {
    public static List<ContactData> list;

    public static String getPhoneTypeDesc(int type) {
        switch (type) {
            case ContactsContract.CommonDataKinds.Phone.TYPE_HOME:
                return "家庭电话";
            case ContactsContract.CommonDataKinds.Phone.TYPE_MOBILE:
                return "手机号码";
            case ContactsContract.CommonDataKinds.Phone.TYPE_WORK:
                return "工作电话";
            case ContactsContract.CommonDataKinds.Phone.TYPE_FAX_HOME:
                return "家庭传真";
            case ContactsContract.CommonDataKinds.Phone.TYPE_FAX_WORK:
                return "工作传真";
            case ContactsContract.CommonDataKinds.Phone.TYPE_MAIN:
                return "主要电话";
            case ContactsContract.CommonDataKinds.Phone.TYPE_OTHER:
                return "其它电话";
            case ContactsContract.CommonDataKinds.Phone.TYPE_CUSTOM:
                return "自定义";
            case ContactsContract.CommonDataKinds.Phone.TYPE_PAGER:
                return "传呼机";
        }
        return "未知";
    }

    public static String getEmailTypeDesc(int type) {
        switch (type) {
            case ContactsContract.CommonDataKinds.Email.TYPE_HOME:
                return "家庭邮件";
            case ContactsContract.CommonDataKinds.Email.TYPE_WORK:
                return "工作邮件";
            case ContactsContract.CommonDataKinds.Email.TYPE_OTHER:
                return "其它邮件";
            case ContactsContract.CommonDataKinds.Email.TYPE_MOBILE:
                return "手机邮件";
        }
        return "自定义";
    }

    public static String getAddressTypeDesc(int type) {

        switch (type) {
            case ContactsContract.CommonDataKinds.StructuredPostal.TYPE_HOME:
                return "家庭地址";
            case ContactsContract.CommonDataKinds.StructuredPostal.TYPE_WORK:
                return "办公地址";
            case ContactsContract.CommonDataKinds.StructuredPostal.TYPE_OTHER:
                return "其它地址";
        }
        return "自定义";
    }

    public static String getBirthdayTypeDesc(int type) {
        switch (type) {
            case ContactsContract.CommonDataKinds.Event.TYPE_ANNIVERSARY:
                return "周年纪念日";
            case ContactsContract.CommonDataKinds.Event.TYPE_BIRTHDAY:
                return "公历生日";
            case ContactsContract.CommonDataKinds.Event.TYPE_OTHER:
                return "其它";
        }
        return "自定义";
    }
    //删除整个通讯录
    public static void deleteAll(Context context) {
        ContentResolver cr = context.getContentResolver();
        try {
            Uri uri = ContactsContract.RawContacts.CONTENT_URI.buildUpon().appendQueryParameter(ContactsContract.CALLER_IS_SYNCADAPTER, "true").build();
            cr.delete(uri, null, null);
        } catch (Exception e) {
            Cursor cursor = cr.query(ContactsContract.Contacts.CONTENT_URI, null, null, null);
            // 如果记录不为空
            if (cursor.getCount() > 0) {
                // 游标初始指向查询结果的第一条记录的上方，执行moveToNext函数会判断
                // 下一条记录是否存在，如果存在，指向下一条记录。否则，返回false
                while (cursor.moveToNext()) {
                    @SuppressLint("Range")
                    String contactId = cursor.getString(cursor.getColumnIndex(ContactsContract.Contacts._ID));
                    Uri raw_uri = Uri.parse("content://com.android.contacts/raw_contacts");
                    Cursor raw_cursor = cr.query(raw_uri, new String[]{ContactsContract.RawContacts._ID}, "contact_id=?", new String[]{contactId}, null);
                    if (raw_cursor.moveToFirst()) {
                        //根据id删除data中的相应数据
                        String[] id = new String[]{String.valueOf(raw_cursor.getInt(0))};
                        cr.delete(raw_uri, "_id=?", id);
                        Uri data_uri = Uri.parse("content://com.android.contacts/data");
                        cr.delete(data_uri, "raw_contact_id=?", id);
                    }
                    raw_cursor.close();
                }
            }
            cursor.close();
        }
    }

    //删除通讯录
    public static void delete(Context context, Long rawContactId){
        ContentResolver contentResolver = context.getContentResolver();
        Uri uri = Uri.parse("content://com.android.contacts/raw_contacts");
        Cursor cursor = contentResolver.query(uri, new String[]{ContactsContract.RawContacts._ID}, "contact_id=?", new String[]{String.valueOf(rawContactId)}, null);
        if (cursor.moveToFirst()) {
            String[] id = new String[]{String.valueOf(cursor.getInt(0))};
            contentResolver.delete(uri, "_id=?", id);
            uri = Uri.parse("content://com.android.contacts/data");
            contentResolver.delete(uri, "raw_contact_id=?", id);
        }
        cursor.close();
    }

    @SuppressLint("Range")
    public static void fetch(Context context) {
        list = new ArrayList<>();

        ContentResolver cr = context.getContentResolver();
        Cursor cur = cr.query(ContactsContract.Contacts.CONTENT_URI, null, null, null, null);
        if (cur.getCount() > 0) {
            try {
                while (cur.moveToNext()) {
                    String id = cur.getString(cur.getColumnIndex(ContactsContract.Contacts._ID));
                    String name = cur.getString(cur.getColumnIndex(ContactsContract.Contacts.DISPLAY_NAME));

                    ContactData contactClass = new ContactData();
                    contactClass.id = Long.parseLong(id);
                    contactClass.name = name;
                    if (cur.getInt(cur.getColumnIndex(ContactsContract.Contacts.HAS_PHONE_NUMBER)) > 0) {
                        Cursor pCur = cr.query(ContactsContract.CommonDataKinds.Phone.CONTENT_URI, null,
                                ContactsContract.CommonDataKinds.Phone.CONTACT_ID + " = ?",
                                new String[]{id}, null);

                        if (pCur.getCount() > 0) {
                            contactClass.phones = new ArrayList<>();
                            //获取手机号码
                            while (pCur.moveToNext()) {
                                String phoneNo = pCur.getString(pCur.getColumnIndex(ContactsContract.CommonDataKinds.Phone.NUMBER));
                                int type = pCur.getInt(pCur.getColumnIndex(ContactsContract.CommonDataKinds.Phone.TYPE));

                                if (phoneNo != null && phoneNo.length() > 0) {
                                    contactClass.addPhone(type, phoneNo);
                                }
                            }
                        }
                        pCur.close();
                    }

                    //获取邮箱地址
                    Cursor emailCur = cr.query(ContactsContract.CommonDataKinds.Email.CONTENT_URI, null,
                            ContactsContract.CommonDataKinds.Email.CONTACT_ID + " = " + id, null, null);
                    if (emailCur.getCount() > 0) {
                        contactClass.emails = new ArrayList<>();
                        while (emailCur.moveToNext()) {
                            int emailType = emailCur.getInt(emailCur.getColumnIndex(ContactsContract.CommonDataKinds.Email.TYPE));
                            String email = emailCur.getString(emailCur.getColumnIndex(ContactsContract.CommonDataKinds.Email.DATA));
                            if (email != null && email.length() > 0) {
                                contactClass.addEmail(emailType, email);
                            }
                        }
                    }
                    emailCur.close();

                    //获取通讯工具号码
                    String whereIm = ContactsContract.CommonDataKinds.Event.MIMETYPE + " = '" + ContactsContract.CommonDataKinds.Im.CONTENT_ITEM_TYPE +"' and " +
                            ContactsContract.Data.CONTACT_ID + " = " + id;

                    Cursor ImCur = cr.query(ContactsContract.Data.CONTENT_URI, null,
                            whereIm, null, null);
                    if (ImCur.getCount() > 0) {
                        contactClass.ims = new ArrayList<>();
                        while (ImCur.moveToNext()) {
                            int imType = ImCur.getInt(ImCur.getColumnIndex(ContactsContract.CommonDataKinds.Im.TYPE));
                            String im = ImCur.getString(ImCur.getColumnIndex(ContactsContract.CommonDataKinds.Im.DATA));
                            if (im != null && im.length() > 0) {
                                contactClass.addIM(imType, im);
                            }
                        }
                    }
                    ImCur.close();

                    // 获取该联系人地址
                    Cursor addressCur = cr.query(ContactsContract.CommonDataKinds.StructuredPostal.CONTENT_URI, null,
                            ContactsContract.CommonDataKinds.StructuredPostal.CONTACT_ID + " = " + id, null, null);
                    if (addressCur.getCount() > 0) {
                        contactClass.addresses = new ArrayList<>();
                        while (addressCur.moveToNext()) {
                            // 遍历所有的地址
                            ContactData.Address address = new ContactData.Address();
                            address.country = addressCur.getString(addressCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredPostal.COUNTRY));
                            address.city = addressCur.getString(addressCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredPostal.CITY));
                            address.street = addressCur.getString(addressCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredPostal.STREET));
                            address.region = addressCur.getString(addressCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredPostal.REGION));
                            address.postalCode = addressCur.getString(addressCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredPostal.POSTCODE));
                            address.formatAddress = addressCur.getString(addressCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredPostal.FORMATTED_ADDRESS));
                            address.type = addressCur.getInt(addressCur.getColumnIndex(ContactsContract.CommonDataKinds.StructuredPostal.TYPE));
                            if (address.formatAddress == null || address.formatAddress.length() <= 0) {
                                address.formatAddress = address.country+address.city+address.region+address.street;
                            }
                            if (address.formatAddress.length() > 0) {
                                contactClass.addAddress(address);
                            }
                        }
                    }
                    addressCur.close();
                    //获取生日
                    String[] columns = {ContactsContract.CommonDataKinds.Event.START_DATE,
                            ContactsContract.CommonDataKinds.Event.TYPE,
                            ContactsContract.CommonDataKinds.Event.MIMETYPE};

                    String where = ContactsContract.CommonDataKinds.Event.MIMETYPE + " = '" + ContactsContract.CommonDataKinds.Event.CONTENT_ITEM_TYPE +"' and " +
                            ContactsContract.Data.CONTACT_ID + " = " + id;

                    Cursor birthdayCur = cr.query(ContactsContract.Data.CONTENT_URI, columns, where, null, ContactsContract.Contacts.DISPLAY_NAME);

                    if (birthdayCur.getCount() > 0) {
                        contactClass.birthdays = new ArrayList<>();
                        while (birthdayCur.moveToNext()) {
                            String birthday = birthdayCur.getString(birthdayCur.getColumnIndex(ContactsContract.CommonDataKinds.Event.START_DATE));
                            int birthdayType = birthdayCur.getInt(birthdayCur.getColumnIndex(ContactsContract.CommonDataKinds.Event.TYPE));
                            contactClass.addBirthday(birthdayType,birthday);
                        }
                        birthdayCur.close();
                    }

                    // 获取备注信息
                    Cursor noteCur = cr.query(ContactsContract.Data.CONTENT_URI,
                            new String[]{ContactsContract.Data._ID, ContactsContract.CommonDataKinds.Note.NOTE},
                            ContactsContract.Data.CONTACT_ID + "=?" + " AND " + ContactsContract.Data.MIMETYPE + "='"
                                    + ContactsContract.CommonDataKinds.Note.CONTENT_ITEM_TYPE + "'", new String[]{id}, null);

                    if (noteCur.getCount() > 0) {
                        while (noteCur.moveToNext()) {
                            contactClass.remark = noteCur.getString(noteCur.getColumnIndex(ContactsContract.CommonDataKinds.Note.NOTE));
                        }
                    }
                    noteCur.close();

                    list.add(contactClass);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        cur.close();
    }
    /**
     * 向系统通讯录插入数据
     */
    public static void add(Context context, ContactData contactData) {
        ContentValues values = new ContentValues();

        // 向RawContacts.CONTENT_URI空值插入，
        // 先获取Android系统返回的rawContactId
        // 后面要基于此id插入值
        ContentResolver cr = context.getContentResolver();
        Uri rawContactUri = cr.insert(ContactsContract.RawContacts.CONTENT_URI, values);
        long rawContactId = ContentUris.parseId(rawContactUri);
        values.clear();

        values.put(ContactsContract.Data.RAW_CONTACT_ID, rawContactId);
        // 内容类型
        values.put(ContactsContract.Data.MIMETYPE, ContactsContract.CommonDataKinds.StructuredName.CONTENT_ITEM_TYPE);
        // 联系人名字
        values.put(ContactsContract.CommonDataKinds.StructuredName.GIVEN_NAME, contactData.name);
        // 向联系人URI添加联系人名字
        cr.insert(ContactsContract.Data.CONTENT_URI, values);

        //插入电话号码
        if (contactData.phones != null && contactData.phones.size() > 0) {
            for (ContactData.Data data : contactData.phones) {
                values.clear();
                values.put(ContactsContract.Data.RAW_CONTACT_ID, rawContactId);
                values.put(ContactsContract.Data.MIMETYPE, ContactsContract.CommonDataKinds.Phone.CONTENT_ITEM_TYPE);
                // 联系人的电话号码
                values.put(ContactsContract.CommonDataKinds.Phone.NUMBER, (String)data.data);
                // 电话类型
                values.put(ContactsContract.CommonDataKinds.Phone.TYPE, data.type);
                cr.insert(ContactsContract.Data.CONTENT_URI, values);
            }
        }

        //插入地址
        if (contactData.addresses != null && contactData.addresses.size() > 0) {
            for (ContactData.Address address : contactData.addresses) {
                values.clear();
                values.put(ContactsContract.Data.RAW_CONTACT_ID, rawContactId);
                values.put(ContactsContract.Data.MIMETYPE, ContactsContract.CommonDataKinds.Phone.CONTENT_ITEM_TYPE);

                values.put(ContactsContract.Data.MIMETYPE, ContactsContract.CommonDataKinds.StructuredPostal.CONTENT_ITEM_TYPE);
                values.put(ContactsContract.CommonDataKinds.StructuredPostal.COUNTRY, address.country);
                values.put(ContactsContract.CommonDataKinds.StructuredPostal.CITY, address.city);
                values.put(ContactsContract.CommonDataKinds.StructuredPostal.REGION, address.region);
                values.put(ContactsContract.CommonDataKinds.StructuredPostal.STREET, address.street);
                values.put(ContactsContract.CommonDataKinds.StructuredPostal.POSTCODE, address.postalCode);
                values.put(ContactsContract.CommonDataKinds.StructuredPostal.FORMATTED_ADDRESS, address.formatAddress);
                // 类型
                values.put(ContactsContract.CommonDataKinds.StructuredPostal.TYPE, address.type);
                cr.insert(ContactsContract.Data.CONTENT_URI, values);
            }
        }

        //插入邮箱
        if (contactData.emails != null && contactData.emails.size() > 0) {
            for (ContactData.Data data : contactData.emails) {
                values.clear();
                values.put(ContactsContract.Data.RAW_CONTACT_ID, rawContactId);
                values.put(ContactsContract.Data.MIMETYPE, ContactsContract.CommonDataKinds.Email.CONTENT_ITEM_TYPE);
                // 联系人的邮件地址
                values.put(ContactsContract.CommonDataKinds.Email.DATA, (String)data.data);
                // 邮件类型
                values.put(ContactsContract.CommonDataKinds.Email.TYPE, data.type);
                cr.insert(ContactsContract.Data.CONTENT_URI, values);
            }
        }

        //插入即时通信
        if (contactData.ims != null && contactData.ims.size() > 0) {
            for (ContactData.Data data : contactData.ims) {
                values.clear();
                values.put(ContactsContract.Data.RAW_CONTACT_ID, rawContactId);
                values.put(ContactsContract.Data.MIMETYPE, ContactsContract.CommonDataKinds.Im.CONTENT_ITEM_TYPE);
                // 即时通信号码
                values.put(ContactsContract.CommonDataKinds.Im.DATA, (String)data.data);
                // 即时通信类型
                values.put(ContactsContract.CommonDataKinds.Im.TYPE, data.type);
                cr.insert(ContactsContract.Data.CONTENT_URI, values);
            }
        }

        //插入生日
        if (contactData.birthdays != null && contactData.birthdays.size() > 0) {
            for (ContactData.Data data : contactData.birthdays) {
                values.clear();
                values.put(ContactsContract.Data.RAW_CONTACT_ID, rawContactId);
                values.put(ContactsContract.Data.MIMETYPE, ContactsContract.CommonDataKinds.Event.CONTENT_ITEM_TYPE);
                // 生日类型
                values.put(ContactsContract.CommonDataKinds.Event.TYPE, data.type);
                values.put(ContactsContract.CommonDataKinds.Event.START_DATE, (String)data.data);
                cr.insert(ContactsContract.Data.CONTENT_URI, values);
            }
        }

        //插入备注
        if (contactData.remark != null) {
            values.clear();
            values.put(ContactsContract.Data.RAW_CONTACT_ID, rawContactId);
            values.put(ContactsContract.Data.MIMETYPE, ContactsContract.CommonDataKinds.Note.CONTENT_ITEM_TYPE);
            values.put(ContactsContract.CommonDataKinds.Note.NOTE, contactData.remark);
            cr.insert(ContactsContract.Data.CONTENT_URI, values);
        }
    }

    public static JSONArray toJSONArray() throws JSONException{
        JSONArray jsonArray = new JSONArray();
        for (ContactData data : list) {
            jsonArray.put(data.toJSONObject());
        }
        return jsonArray;
    }
}