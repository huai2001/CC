
#include <stdlib.h>
#include <stdio.h>
#include <libcc.h>
#include <cc/json/json.h>
#include <locale.h>

#include "cJSON/cJSON.h"

tchar_t *json_string = _T("{")
_T("\"array\": [1,2,  4  ,  3,5,6,true    ,{")
_T("\"a1\": \"#$%\",")
_T("\"c2\"  : \"HY\\ud83c\\udf0a\", ")
_T("\"e3\": \"f3\"},7,8,9,0],")
_T("\"string1\": \"Hello\tWorld1\"  ,  ")
_T("\"boolean\": true       ,")
_T("\"null\" : null,")
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
    _cc_json_t *json = _cc_parse_json((const tchar_t*)json_string);//
    _cc_buf_t *buf = NULL;
    //json = _cc_json_add_object(_CC_JSON_OBJECT_, NULL);
    if (json) {
        _cc_json_add_string(json, _T("nickname"), _T("abc中文\""), true);
        _cc_json_add_string(json, _T("nickname1"), _T("abc中文1\""), true);
        _cc_json_add_string(json, _T("nickname2"), _T("abc中文2\""), true);
        _cc_json_add_number(json, _T("sex"), 2, true);
        _cc_json_add_string(json, _T("language"), _T("zh_CN2"), true);
        _cc_json_add_boolean(json, _T("status"), true, true);
        
        _cc_json_object_remove(json, _T("nickname1"));
        json2 = _cc_json_object_find(json, _T("array"));
        if (json2) {
            _cc_json_array_remove(json2, 1);
        }
        buf = _cc_print_json(json);
        printf("%s\n\n", (char*)_cc_buf_bytes(buf));
        _cc_destroy_buf(&buf);
    } else {
		printf("%s\n", _cc_json_error());
	}
	/*
    fputs("\n\n",fp);
    {
        cJSON *j = cJSON_CreateObject();
        cJSON *qiu = cJSON_CreateString("abc\\u4e2dB\\u56fd\"");
        cJSON_AddItemToObject(j, "nick\"name", qiu);
        fprintf(fp,"%s\n", cJSON_Print(j));
        cJSON_Delete(j);
	}*/
	_cc_destroy_json(&json);
    
}
uint32_t _cc_hex2dec2(const char_t *input, size_t i);
#define LOOP_MAX 10
/* Parse text to JSON, then render back to text, and print! */
int main(int argc, char* argv[]) {
    /*, */
    byte_t c = 0;
    int32_t i = 0, j = 0;
    clock_t start, end;
    _cc_json_t *json = NULL;
    cJSON *cjson = NULL;
    
    setlocale(LC_ALL, "chs");
    
    _tprintf(_T("%s\n"),_T(_CC_COMPILER_NAME_));
    
    start = clock();
    for (i = 0; i < LOOP_MAX; i++) {
        cjson = cJSON_Parse(json_string);
        if (cjson == NULL) {
            printf("cJSON_Parse error %s\n",cJSON_GetErrorPtr());
            break;
        }

		//printf("%s\n", cJSON_Print(cjson));
        //for (j = 0; j <LOOP_MAX; j++)
        //    cJSON_GetObjectItem(cjson, "nickname");
        //printf("cJSON:%s\n",cJSON_GetObjectItem(cjson, "nickname")->valuestring);
        cJSON_Delete(cjson);
    }
    
    end = clock();
    printf("cJSON_Parse: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
   
    start = clock();
    for (i = 0; i < LOOP_MAX; i++) {
        json = _cc_parse_json((const tchar_t*)json_string);
        if (json == NULL) {
            printf("_cc_parse_json error %s\n",_cc_json_error());
            break;
        }
        //printf("json:%s\n",_cc_json_object_find_string(json, "nickname"));
        //for (j = 0; j <LOOP_MAX; j++)
        //   _cc_json_object_find_string(json, _T("nickname"));
		//_cc_save_json_file(json, stdout);
        _cc_destroy_json(&json);
    }
    end = clock();
    printf("CC JSON Parse: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
    
    json_P();

    while ((c = getchar()) != 'q');
    
    return 0; 
} 
