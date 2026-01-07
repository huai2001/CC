#include <libcc/ini.h>
#include <stdio.h>
#include <string.h>

struct {const tchar_t *data; size_t length;} ini_content = {_CC_STRING("[section1]\nkey1 = value1 #adfsd\n;abcd  \nkey2 = \"value2  \"\n[section2]\nkey3 = value3")};

static void test_ini_parse() {
    _cc_ini_t* ini = _cc_parse_ini(ini_content.data, ini_content.length);
    
    if (ini == NULL) {
        printf("Failed to parse INI content\n");
        return;
    }
    
    _cc_ini_t* section1 = _cc_ini_find(ini, _T("section1"));
    if (section1 == NULL) {
        printf("Failed to find section1\n");
        _cc_free_ini(ini);
        return;
    }
    
    const tchar_t* value1 = _cc_ini_find_string(section1, _T("key1"));
    if (value1 == NULL || _tcscmp(value1, _T("value1")) != 0) {
        printf("Failed to find key1 or incorrect value\n");
        _cc_free_ini(ini);
        return;
    }
    
    const tchar_t* value2 = _cc_ini_find_string(section1, _T("key2"));
    if (value2 == NULL || _tcscmp(value2, _T("value2")) != 0) {
        printf("Failed to find key2 or incorrect value\n");
        _cc_free_ini(ini);
        return;
    }
    
    _cc_ini_t* section2 = _cc_ini_find(ini, _T("section2"));
    if (section2 == NULL) {
        printf("Failed to find section2\n");
        _cc_free_ini(ini);
        return;
    }
    
    const tchar_t* value3 = _cc_ini_find_string(section2, _T("key3"));
    if (value3 == NULL || _tcscmp(value3, _T("value3")) != 0) {
        printf("Failed to find key3 or incorrect value\n");
        _cc_free_ini(ini);
        return;
    }
    
    printf("INI parsing test passed\n");

    _cc_buf_t dump;
    _cc_dump_ini(ini, &dump);
    FILE *fp = fopen("test.ini", "w");
    assert(fp != NULL);
    fwrite(dump.bytes, 1, dump.length, fp);
    fclose(fp);
    _cc_free_buf(&dump);

    _cc_free_ini(ini);
}

static void test_ini_from_file() {
    _cc_ini_t* ini = _cc_ini_from_file("test.ini");
    
    if (ini == NULL) {
        printf("Failed to parse INI file\n");
        return;
    }
    
    _cc_ini_t* section = _cc_ini_find(ini, _T("section1"));
    if (section == NULL) {
        printf("Failed to find section1 in file\n");
        _cc_free_ini(ini);
        return;
    }
    
    const tchar_t* value = _cc_ini_find_string(section, _T("key1"));
    if (value == NULL || _tcscmp(value, _T("value1")) != 0) {
        printf("Failed to find key1 or incorrect value in file\n");
        _cc_free_ini(ini);
        return;
    }
    
    printf("INI file parsing test passed\n");
    _cc_free_ini(ini);
}

static void test_ini_error_handling() {
    const tchar_t* invalid_ini = _T("[section1\nkey1 = value1");
    _cc_ini_t* ini = _cc_parse_ini(invalid_ini, _tcslen(invalid_ini));
    
    if (ini != NULL) {
        printf("Failed to detect invalid INI content\n");
        _cc_free_ini(ini);
        return;
    }
    
    const tchar_t* error = _cc_ini_error();
    if (error == NULL) {
        printf("Failed to retrieve error message\n");
        return;
    }
    
    printf("Error handling test passed: %s\n", error);
}

int main() {
    test_ini_parse();
    test_ini_error_handling();
    test_ini_from_file();
    return 0;
}