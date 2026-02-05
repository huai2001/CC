#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libcc/json.h>
struct {const tchar_t *data; size_t length;} json_content = {_CC_STRING("{\"code\":200,\"data\":{\"empty\":\"\",\"c\":\"Hello\\n中国\",\"e\":\"1728576000000\",\"s\":\"vIOiQyA7giWK\",\"t\":\"154ff80489dacae94c45247235cd8083\",\"u\":\"js/dfxaf3-ba348b9e.js\",\"a\":[1,2,3,4,5,6,7,8,9,0]},\"id\":\"5bb5f254-36a8-4267-94fb-22287d0d2477\",\"message\":\"\\n\"}")};
/* Helper function to print test results */
void print_test_result(const char *test_name, int passed) {
    printf("%s: %s\n", test_name, passed ? "PASSED" : "FAILED");
}

/* Test cases */
void test_json_alloc_object() {
    _cc_json_t *obj = _cc_json_alloc_object(_CC_JSON_OBJECT_, "test");
    assert(obj != NULL);
    assert(obj->type == _CC_JSON_OBJECT_);
    assert(strcmp(obj->name, "test") == 0);
    _cc_free_json(obj);
    print_test_result(__func__, 1);
}

void test_json_alloc_array() {
    _cc_json_t *arr = _cc_json_alloc_array("test_array", 5);
    assert(arr != NULL);
    assert(arr->type == _CC_JSON_ARRAY_);
    assert(_cc_array_available(arr->element.uni_array) == 5);
    _cc_free_json(arr);
    print_test_result(__func__, 1);
}

void test_json_add_boolean() {
    _cc_json_t *obj = _cc_json_alloc_object(_CC_JSON_OBJECT_, "root");
    _cc_json_t *bool_val = _cc_json_add_boolean(obj, "bool_key", true);
    assert(bool_val != NULL);
    assert(bool_val->type == _CC_JSON_BOOLEAN_);
    assert(bool_val->element.uni_boolean == true);
    _cc_free_json(obj);
    print_test_result(__func__, 1);
}

void test_json_add_number() {
    _cc_json_t *obj = _cc_json_alloc_object(_CC_JSON_OBJECT_, "root");
    _cc_json_t *num_val = _cc_json_add_number(obj, "num_key", 42);
    assert(num_val != NULL);
    assert(num_val->type == _CC_JSON_INT_);
    assert(num_val->element.uni_int == 42);
    _cc_free_json(obj);
    print_test_result(__func__, 1);
}

void test_json_add_string() {
    _cc_json_t *obj = _cc_json_alloc_object(_CC_JSON_OBJECT_, "root");
    _cc_json_t *str_val = _cc_json_add_string(obj, "str_key", "test_string");
    assert(str_val != NULL);
    assert(str_val->type == _CC_JSON_STRING_);
    assert(strcmp(str_val->element.uni_string, "test_string") == 0);
    _cc_free_json(obj);
    print_test_result(__func__, 1);
}

void test_json_object_find() {
    _cc_json_t *obj = _cc_json_alloc_object(_CC_JSON_OBJECT_, "root");
    _cc_json_add_number(obj, "num_key", 42);
    _cc_json_t *found = _cc_json_object_find(obj, "num_key");
    assert(found != NULL);
    assert(found->type == _CC_JSON_INT_);
    assert(found->element.uni_int == 42);
    _cc_free_json(obj);
    print_test_result(__func__, 1);
}

void test_json_array_push() {
    _cc_json_t *arr = _cc_json_alloc_array("test_array", 2);
    _cc_json_t *item = _cc_json_alloc_object(_CC_JSON_INT_, "item");
    item->element.uni_int = 10;
    assert(_cc_json_array_push(arr, item));
    assert(_cc_array_length(arr->element.uni_array) == 1);
    _cc_free_json(arr);
    print_test_result(__func__, 1);
}

void test_json_array_remove() {
    _cc_json_t *arr = _cc_json_alloc_array("test_array", 2);
    _cc_json_t *item = _cc_json_alloc_object(_CC_JSON_INT_, "item");
    item->element.uni_int = 10;
    _cc_json_array_push(arr, item);
    assert(_cc_array_length(arr->element.uni_array) == 1);
    assert(_cc_json_array_remove(arr, 0));
    assert(_cc_array_length(arr->element.uni_array) == 0);
    _cc_free_json(arr);
    print_test_result(__func__, 1);
}

void test_json_parse() {
    _cc_buf_t dump;
    _cc_json_t *parsed = _cc_json_parse(json_content.data, json_content.length);
    assert(parsed != NULL);
    assert(parsed->type == _CC_JSON_OBJECT_);
    _cc_json_dump(parsed,&dump);
    
    _cc_free_json(parsed);
    print_test_result(__func__, 1);

    FILE *fp = _tfopen("test.json", _T("wb"));
    assert(fp != NULL);
    fwrite(dump.bytes, 1, dump.length, fp);
    fclose(fp);
    _cc_free_buf(&dump);
}

void test_json_from_file() {
    // Assuming a test file "test.json" exists with valid JSON content
    _cc_json_t *json = _cc_json_from_file("test.json");
    assert(json != NULL);
    _cc_free_json(json);
    print_test_result(__func__, 1);
}

/* Main function */
int main() {
    printf("Running JSON tests...\n");
    test_json_alloc_object();
    test_json_alloc_array();
    test_json_add_boolean();
    test_json_add_number();
    test_json_add_string();
    test_json_object_find();
    test_json_array_push();
    test_json_array_remove();
    test_json_parse();
    test_json_from_file();
    printf("All tests completed.\n");
    return 0;
}