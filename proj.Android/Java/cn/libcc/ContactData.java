package cn.libcc;

import android.provider.ContactsContract;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;


public class ContactData implements Serializable {
    public long id = 0;
    public String name = null;
    public List<Data> phones = null;
    public List<Address> addresses = null;
    public List<Data> emails = null;
    public List<Data> ims;//即时信息
    public List<Data> birthdays;
    public String remark;//备注

    public void addPhone(int type, String number) {
        if (number.isEmpty() || number.length() == 0) {
            return;
        }
        Data data = new Data(type, number);
        this.phones.add(data);
    }

    public void addEmail(int type, String email) {
        if (email.isEmpty() || email.length() == 0) {
            return;
        }
        Data data = new Data(type, email);
        this.emails.add(data);
    }

    public void addIM(int type, String im) {
        if (im.isEmpty() || im.length() == 0) {
            return;
        }
        Data data = new Data(type, im);
        this.ims.add(data);
    }
    public void addAddress(Address address) {
        this.addresses.add(address);
    }

    public void addBirthday(int type, String birthday) {
        Data data = new Data(type, birthday);
        this.birthdays.add(data);
    }

    public boolean parseJSON(List<Data> list, JSONArray jsonArray) throws JSONException {
        int count = 0;
        for (int i = 0; i < jsonArray.length(); i++) {
            JSONObject obj = jsonArray.getJSONObject(i);
            if (obj == null) {
                continue;
            }

            Data data = new Data();
            if (data.parseJSON(obj)) {
                list.add(data);
                count++;
            }
        }
        return count > 0;
    }

    public boolean parseJSONAddress(List<Address> list, JSONArray jsonArray) throws JSONException {
        int count = 0;
        for (int i = 0; i < jsonArray.length(); i++) {
            JSONObject obj = jsonArray.getJSONObject(i);
            if (obj == null) {
                continue;
            }

            Address data = new Address();
            if (data.parseJSON(obj)) {
                list.add(data);
                count++;
            }
        }
        return count > 0;
    }
    public static class Data implements Serializable {
        public int type;
        public String data;

        public Data() {
            type = 0;
            data = "";
        }

        public Data(int type, String data) {
            this.type = type;
            this.data = data;
        }
        public JSONObject toJSONObject() {
            try {
                JSONObject jsonObject = new JSONObject();
                jsonObject.put("type", this.type);
                jsonObject.put("data", this.data);
                return jsonObject;
            } catch (Exception e) {
                e.printStackTrace();
            }
            return null;
        }

        public boolean parseJSON(JSONObject json) throws JSONException {
            if (!json.has("type") && !json.has("data")) {
                return false;
            }
            this.type = json.getInt("type");
            this.data = json.getString("data");
            return true;
        }
    }

    public static class Address implements Serializable {
        public int type;
        public String country = null;//国家
        public String province = null;//省
        public String city = null;//城市
        public String region = null;//区
        public String street = null;//街道
        public String postalCode = null;//邮编
        public String formatAddress = null;//完整地址
        public Address() {
            this.country = "";
            this.province = "";
            this.city = "";
            this.region = "";
            this.street = "";
            this.postalCode = "";
            this.formatAddress = "";
        }
        public JSONObject toJSONObject() {
            try {
                JSONObject jsonObject = new JSONObject();
                jsonObject.put("type", this.type);
                jsonObject.put("country", this.country);
                jsonObject.put("province", this.province);
                jsonObject.put("city", this.city);
                jsonObject.put("region", this.region);
                jsonObject.put("street", this.street);
                jsonObject.put("postalCode", this.postalCode);
                jsonObject.put("formatAddress", this.formatAddress);
                return jsonObject;
            } catch (Exception e) {
                e.printStackTrace();
            }
            return null;
        }
        public boolean parseJSON(JSONObject json) throws JSONException {
            if (!json.has("type")) {
                return false;
            }

            this.type = json.getInt("type");
            this.country = json.getString("country");
            this.province = json.getString("province");
            this.city = json.getString("city");
            this.region = json.getString("region");
            this.street = json.getString("street");
            this.postalCode = json.getString("postalCode");
            this.formatAddress = json.getString("formatAddress");
            return true;
        }
    }

    public JSONObject toJSONObject() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        //插入电话号码
        if (this.phones != null && this.phones.size() > 0) {
            JSONArray jsonArray = new JSONArray();
            for (ContactData.Data data : this.phones) {
                jsonArray.put(data.toJSONObject());
            }
            jsonObject.put("phone", jsonArray);
        }

        //插入地址
        if (this.addresses != null && this.addresses.size() > 0) {
            JSONArray jsonArray = new JSONArray();
            for (ContactData.Address data : this.addresses) {
                jsonArray.put(data.toJSONObject());
            }
            jsonObject.put("address", jsonArray);
        }

        //插入邮箱
        if (this.emails != null && this.emails.size() > 0) {
            JSONArray jsonArray = new JSONArray();
            for (ContactData.Data data : this.emails) {
                jsonArray.put(data.toJSONObject());
            }
            jsonObject.put("email", jsonArray);
        }

        //插入即时通信
        if (this.ims != null && this.ims.size() > 0) {
            JSONArray jsonArray = new JSONArray();
            for (ContactData.Data data : this.ims) {
                jsonArray.put(data.toJSONObject());
            }
            jsonObject.put("IM", jsonArray);
        }

        //插入生日
        if (this.birthdays != null && this.birthdays.size() > 0) {
            JSONArray jsonArray = new JSONArray();
            for (ContactData.Data data : this.emails) {
                jsonArray.put(data.toJSONObject());
            }
            jsonObject.put("birthday", jsonArray);
        }

        jsonObject.put("name", this.name);
        //插入备注
        if (this.remark != null) {
            jsonObject.put("remark", this.remark);
        }
        return jsonObject;
    }
}
