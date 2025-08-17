
#include <stdlib.h>
#include <stdio.h>
#include <libcc.h>
#include <libcc/widgets/widgets.h>
#include <locale.h>

_cc_String_t json_string = _cc_String(
    _T("{")
    _T("\"array\": [1,2,  4  ,  3,5,6,true    ,{")
    _T("\"a1\": \"#$%\",")
    _T("\"c2\"  : \"HY\\ud83c\\udf0a\", ")
    _T("\"e3\": \"f3\"},7,8,9,0],")
    _T("\"string1\": \"Hello\tWorld1\"  ,  ")
    _T("\"boolean\": true       ,")
    _T("\"nullptr\" : null,")
    _T("\"number\" : 123.087  ,  ")
    _T("\"object\" : {")
    _T("\"a\": \"b\",")
    _T("\"c1\": [1,2,3,4,5,6,7,8,9,0],  ")
    _T("\"c2\": \"c2\",  ")
    _T("\"c3\": \"c3\",  ")
    _T("\"c4\": \"c4\",  ")
    _T("\"c5\": {  ")
    _T("\"c1\": \"c1\",  ")
    _T("\"c2\": \"c2\",  ")
    _T("\"c3\": \"c3\",  ")
    _T("\"c4\": \"c4\"  ")
    _T("},")
    _T("\"e\": \"f\"}  , ")
    _T("\"string2\": \"Hello\tWorld2\",  ")
    _T("\"boolen\":false,\"g\":[\"djfwjewdiiwdb\",\"djfwjewdiiwdb\",112233,12.01356789],\"empty\":{\"id\":1200,")
    _T("\"num\":10.12345678},\"openid\":\"oLK5hwN2GWidWiRF50smzRG1Wclo\",\"nickname\":\"{C.LUA}abc\u4e2dB\u56fd\",")
    _T("\"sex\":1,\"language\":\"zh_CN\",\"city\":\"Wenzhou\",\"province\":\"Zhejiang\",\"country\":\"CN\",")
    _T("\"headimgurl\":\"http:\\/\\/wx.qlogo.cn\\/mmopen\\/")
    _T("PiajxSqBRaEJBCOs8EUdkEchxl8RH3n57sfNCZWkjz7LkIfUhVxVjL6zIDpCWIOp7Ah5XxIEwOzFHVsfmAEKVrw\\/")
    _T("0\",\"privilege\":[],\"unionid\":\"oYjZdvwS0wEsyihGDRQAjObaoTk0\"}"));

void json_P() {
    _cc_json_t *json2;
    _cc_json_t *json = _cc_json_parse((const tchar_t *)json_string.data, json_string.length); //
    _cc_buf_t buf;

    //json = _cc_json_alloc_object(_CC_JSON_OBJECT_, nullptr);
    _cc_logger_error("test");

    if (json) {
        //_cc_json_add_string(json, _T("nickname"), _T("abc中文\""));
        //_cc_json_add_string(json, _T("nickname1"), _T("abc中文1\""));
        //_cc_json_add_string(json, _T("nickname2"), _T("abc中文2\""));
        //_cc_json_add_number(json, _T("sex"), 2);
        //_cc_json_add_string(json, _T("language"), _T("zh_CN2"));
        //_cc_json_add_boolean(json, _T("status"), true);
        
        //_cc_json_object_remove(json, _T("nickname1"));
        json2 = _cc_json_object_find(json, _T("array"));
        if (json2) {
            //_cc_json_array_remove(json2, 1);
        }
        _cc_json_dump(json,buf);
        printf("%s\n\n", (char*)_cc_buf_stringify(&buf));
        _cc_free_buf(&buf);
    } else {
		printf("%s\n", _cc_json_error());
	}
	_cc_free_json(json);
    
}

#define LOOP_MAX 1
/* Parse text to JSON, then render back to text, and print! */
int main(int argc, char* argv[]) {
    /*, */
    int32_t i = 0;
    clock_t start, end;
    _cc_json_t *json = nullptr;
    char *a = _cc_malloc(sizeof(tchar_t) * 10);
    _cc_install_socket();
    //setlocale(LC_ALL, "chs");
    SetConsoleOutputCP(65001);
#ifdef __CC_WINDOWS__
    _cc_open_syslog(_CC_LOG_FACILITY_USER_, "json5",_T("127.0.0.1"), _CC_PORT_SYSLOG_);
    _widget_open_syslog(_CC_LOG_FACILITY_USER_,_T("127.0.0.1"), _CC_PORT_SYSLOG_);
#else
    _cc_open_syslog(_CC_LOG_FACILITY_USER_, "json5", nullptr, _CC_PORT_SYSLOG_);
    _widget_open_syslog(_CC_LOG_FACILITY_USER_, nullptr, _CC_PORT_SYSLOG_);

#endif
    _tprintf(_T("%s\n"),_T(_CC_COMPILER_NAME_));
    //start = clock();
    //for (i = 0; i < LOOP_MAX; i++) {
    //    json = _cc_json_parse((const tchar_t*)json_string, -1);
    //    if (json == nullptr) {
    //        printf("_cc_json_parse error %s\n",_cc_json_error());
    //        break;
    //    }
    //    //printf("json:%s\n",_cc_json_object_find_string(json, "nickname"));
    //    //for (j = 0; j <LOOP_MAX; j++)
    //    //   _cc_json_object_find_string(json, _T("nickname"));
    //    _cc_free_json(json);
    //}
    //end = clock();
    //_cc_loggerA_info("CC JSON Parse: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
    json_P();

    _cc_free(a);
    _widget_close_syslog();
    _cc_close_syslog();
    _cc_uninstall_socket();
    system("pause");
    
    return 0; 
}