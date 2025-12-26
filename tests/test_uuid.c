#include <libcc/uuid.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_uuid_generation() {
    _cc_uuid_t uuid;
    _cc_uuid(&uuid);
    tchar_t buf[37];
    int32_t result = _cc_uuid_lower(&uuid, buf, sizeof(buf));
    assert(result == 36);
    assert(strlen(buf) == 36);
    printf("UUID generation test passed!\n");
}

void test_uuid_lower_format() {
    _cc_uuid_t uuid;
    _cc_uuid(&uuid);
    tchar_t buf[37];
    int32_t result = _cc_uuid_lower(&uuid, buf, sizeof(buf));
    assert(result == 36);
    assert(strchr(buf, '-') != NULL);
    printf("UUID lower format test passed!\n");
}

void test_uuid_upper_format() {
    _cc_uuid_t uuid;
    _cc_uuid(&uuid);
    tchar_t buf[37];
    int32_t result = _cc_uuid_upper(&uuid, buf, sizeof(buf));
    assert(result == 36);
    assert(strchr(buf, '-') != NULL);
    printf("UUID upper format test passed!\n");
}

void test_uuid_to_bytes() {
    _cc_uuid_t uuid;
    _cc_uuid(&uuid);
    tchar_t buf[37];
    _cc_uuid_lower(&uuid, buf, sizeof(buf));
    _cc_uuid_t uuid_copy;
    _cc_uuid_to_bytes(&uuid_copy, buf);
    assert(memcmp(&uuid, &uuid_copy, sizeof(_cc_uuid_t)) == 0);
    printf("UUID to bytes test passed!\n");
}

void test_uuid_edge_cases() {
    _cc_uuid_t uuid;
    tchar_t buf[37];
    memset(&uuid, 0, sizeof(_cc_uuid_t));
    int32_t result = _cc_uuid_lower(&uuid, buf, sizeof(buf));
    assert(result == 36);
    printf("UUID edge case test passed!\n");
}

int main() {
    test_uuid_generation();
    test_uuid_lower_format();
    test_uuid_upper_format();
    test_uuid_to_bytes();
    test_uuid_edge_cases();
    return 0;
}