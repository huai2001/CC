
#include <stdlib.h>
#include <stdio.h>
#include <libcc.h>
#include <libcc/widgets/widgets.h>
#include <locale.h>

tchar_t *json_string = _T("{")
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
_T("\"boolen\":false,\"g\":[\"djfwjewdiiwdb\",\"djfwjewdiiwdb\",112233,12.01356789],\"empty\":{\"id\":1200,\"num\":10.12345678},\"openid\":\"oLK5hwN2GWidWiRF50smzRG1Wclo\",\"nickname\":\"{C.LUA}abc\u4e2dB\u56fd\",\"sex\":1,\"language\":\"zh_CN\",\"city\":\"Wenzhou\",\"province\":\"Zhejiang\",\"country\":\"CN\",\"headimgurl\":\"http:\\/\\/wx.qlogo.cn\\/mmopen\\/PiajxSqBRaEJBCOs8EUdkEchxl8RH3n57sfNCZWkjz7LkIfUhVxVjL6zIDpCWIOp7Ah5XxIEwOzFHVsfmAEKVrw\\/0\",\"privilege\":[],\"unionid\":\"oYjZdvwS0wEsyihGDRQAjObaoTk0\"}");

void json_P() {
    _cc_json_t *json2;
    _cc_json_t *json = _cc_json_parse((const tchar_t*)json_string, -1);//
    _cc_buf_t *buf = nullptr;

    //json = _cc_json_alloc_object(_CC_JSON_OBJECT_, nullptr);
    _cc_logger_error("test");

    if (json) {
        _cc_json_add_string(json, _T("nickname"), _T("abc中文\""));
        _cc_json_add_string(json, _T("nickname1"), _T("abc中文1\""));
        _cc_json_add_string(json, _T("nickname2"), _T("abc中文2\""));
        _cc_json_add_number(json, _T("sex"), 2);
        _cc_json_add_string(json, _T("language"), _T("zh_CN2"));
        _cc_json_add_boolean(json, _T("status"), true);
        
        _cc_json_object_remove(json, _T("nickname1"));
        json2 = _cc_json_object_find(json, _T("array"));
        if (json2) {
            _cc_json_array_remove(json2, 1);
        }
        buf = _cc_json_dump(json);
        printf("%s\n\n", (char*)_cc_buf_bytes(buf));
        _cc_destroy_buf(&buf);
    } else {
		printf("%s\n", _cc_json_error());
	}
	_cc_destroy_json(&json);
    
}

#define LOOP_MAX 1
/* Parse text to JSON, then render back to text, and print! */
int main(int argc, char* argv[]) {
    /*, */
    int32_t i = 0;
    clock_t start, end;
    _cc_json_t *json = nullptr;
    
    _cc_install_memory_tracked();
    setlocale(LC_ALL, "chs");
    SetConsoleOutputCP(65001);

    _tprintf(_T("%s\n"),_T(_CC_COMPILER_NAME_));
    
    start = clock();
    for (i = 0; i < LOOP_MAX; i++) {
        json = _cc_json_parse((const tchar_t*)json_string, -1);
        if (json == nullptr) {
            printf("_cc_json_parse error %s\n",_cc_json_error());
            break;
        }
        //printf("json:%s\n",_cc_json_object_find_string(json, "nickname"));
        //for (j = 0; j <LOOP_MAX; j++)
        //   _cc_json_object_find_string(json, _T("nickname"));
        _cc_destroy_json(&json);
    }
    end = clock();
    printf("CC JSON Parse: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
    
    json_P();

    _cc_uninstall_memory_tracked();
    system("pause");
    
    return 0; 
}