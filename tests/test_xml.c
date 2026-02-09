#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libcc/xml.h>

    struct {const tchar_t *data; size_t length;} xml_content = {_CC_STRING("<?xml version=\"1.0\" encoding=\"UTF-8\"?><root attr=\"value\"><code>200</code><data><c>Hello\\nWorld</c><e>1728576000000</e><s>vIOiQyA7giWK</s><t>154ff80489dacae94c45247235cd8083</t><u>js/dfxaf3-ba348b9e.js</u><array><a>1</a><a>2</a><a>3</a><a>4</a><a>5</a><a>6</a><a>7</a><a>8</a><a>9</a><a>0</a></array></data><id>5bb5f254-36a8-4267-94fb-22287d0d2477</id><message><![CDATA[This is <b>bold</b> text & more!]]></message></root>")};
/* Helper function to print test results */
void print_test_result(const char *test_name, int passed) {
    printf("%s: %s\n", test_name, passed ? "PASSED" : "FAILED");
}

/* Test cases */
void test_xml_new_element() {
    _cc_xml_t *xml = _cc_alloc_xml_element(_CC_XML_CHILD_);
    assert(xml != NULL);
    assert(xml->type == _CC_XML_CHILD_);
    _cc_free_xml(xml);
    print_test_result(__func__, 1);
}

void test_xml_parse() {
    _cc_xml_t *xml = _cc_xml_parse(xml_content.data, xml_content.length);
    assert(xml != NULL);
    _cc_free_xml(xml);
    print_test_result(__func__, 1);
}

void test_xml_element_append() {
    _cc_xml_t *parent = _cc_alloc_xml_element(_CC_XML_CHILD_);
    _cc_xml_t *child = _cc_alloc_xml_element(_CC_XML_CHILD_);
    assert(_cc_xml_element_append(parent, child));
    _cc_free_xml(parent);
    print_test_result(__func__, 1);
}

void test_xml_element_find() {
    _cc_xml_t *xml = _cc_xml_parse(xml_content.data, xml_content.length);
    _cc_xml_t *found = _cc_xml_element_find(xml, "root/message");
    assert(found != NULL);
    _cc_free_xml(xml);
    print_test_result(__func__, 1);
}

void test_xml_element_text() {
    _cc_xml_t *xml = _cc_xml_parse(xml_content.data, xml_content.length);
    _cc_xml_t *found = _cc_xml_element_find(xml, "root/message");
    const tchar_t *text = _cc_xml_element_text(found);
    assert(text != NULL);
    _cc_free_xml(xml);
    print_test_result(__func__, 1);
}

void test_xml_element_attr_find() {
    _cc_buf_t dump;
    _cc_xml_t *xml = _cc_xml_parse(xml_content.data, xml_content.length);
    _cc_xml_t *found = _cc_xml_element_find(xml, "root");
    const tchar_t *attr = _cc_xml_element_attr(xml, "version");
    assert(found != NULL);
    assert(attr != NULL);
    const tchar_t *attr2 = _cc_xml_element_attr(found, "attr");
    assert(attr2 != NULL);
    _cc_dump_xml(xml, &dump);
    _cc_free_xml(xml);

    FILE *fp = _tfopen("test.xml", _T("w"));
    assert(fp != NULL);
    fwrite(dump.bytes, 1, dump.length, fp);
    fclose(fp);
    _cc_free_buf(&dump);
    print_test_result(__func__, 1);
}

void test_xml_element_set_attr() {
    _cc_xml_t *xml = _cc_alloc_xml_element(_CC_XML_CHILD_);
    assert(_cc_xml_element_set_attr(xml, "attr", "value"));
    _cc_free_xml(xml);
    print_test_result(__func__, 1);
}

void test_xml_from_file() {
    const tchar_t *file_name = "test.xml";
    _cc_xml_t *xml = _cc_xml_from_file(file_name);
    assert(xml != NULL);
    _cc_free_xml(xml);
    print_test_result(__func__, 1);
}

int main() {
    test_xml_new_element();
    test_xml_parse();
    test_xml_element_append();
    test_xml_element_find();
    test_xml_element_text();
    test_xml_element_attr_find();
    test_xml_element_set_attr();
    test_xml_from_file();
    return 0;
}