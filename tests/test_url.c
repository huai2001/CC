#include <stdio.h>
#include <string.h>
#include <libcc/url.h>

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("Assertion failed: %s\n", message); \
            return 1; \
        } \
    } while (0)

/* Test cases for URL parsing */
int test_parse_url_scheme() {
    _cc_url_t url;
    const tchar_t *http_url = _T("http://example.com/path/index.html?query=1");
    const tchar_t *https_url = _T("https://example.com");
    const tchar_t *ftp_url = _T("ftp://example.com");
    const tchar_t *invalid_url = _T("invalid://example.com");

    /* Test HTTP scheme */
    ASSERT(_cc_parse_url(&url, http_url), "HTTP URL parsing failed");
    ASSERT(url.scheme.ident == _CC_SCHEME_HTTP_, "HTTP scheme ident mismatch");
    ASSERT(_tcscmp(url.scheme.value, _T("HTTP")) == 0, "HTTP scheme value mismatch");
    ASSERT(_tcscmp(url.request, _T("/path/index.html?query=1")) == 0, "HTTP request mismatch");
    ASSERT(_tcscmp(url.path, _T("/path/index.html")) == 0, "HTTP path mismatch");
    _cc_free_url(&url);

    /* Test HTTPS scheme */
    ASSERT(_cc_parse_url(&url, https_url), "HTTPS URL parsing failed");
    ASSERT(url.scheme.ident == _CC_SCHEME_HTTPS_, "HTTPS scheme ident mismatch");
    ASSERT(_tcscmp(url.scheme.value, _T("HTTPS")) == 0, "HTTPS scheme value mismatch");
    _cc_free_url(&url);

    /* Test FTP scheme */
    ASSERT(_cc_parse_url(&url, ftp_url), "FTP URL parsing failed");
    ASSERT(url.scheme.ident == _CC_SCHEME_FTP_, "FTP scheme ident mismatch");
    ASSERT(_tcscmp(url.scheme.value, _T("FTP")) == 0, "FTP scheme value mismatch");
    _cc_free_url(&url);

    /* Test invalid scheme */
    ASSERT(_cc_parse_url(&url, invalid_url), "Invalid URL parsing failed");
    ASSERT(url.scheme.ident == _CC_SCHEME_UNKNOWN_, "Invalid scheme ident mismatch");
    ASSERT(_tcscmp(url.scheme.value, _T("invalid")) == 0, "Invalid scheme value mismatch");
    _cc_free_url(&url);

    return 0;
}

int test_parse_url_with_credentials() {
    _cc_url_t url;
    const tchar_t *url_with_creds = _T("http://user:pass@example.com");

    ASSERT(_cc_parse_url(&url, url_with_creds), "URL with credentials parsing failed");
    ASSERT(_tcscmp(url.username, _T("user")) == 0, "Username mismatch");
    ASSERT(_tcscmp(url.password, _T("pass")) == 0, "Password mismatch");
    _cc_free_url(&url);

    return 0;
}

int test_parse_ipv6_url() {
    _cc_url_t url;
    const tchar_t *ipv6_url = _T("http://[2001:db8::1]:8080");

    ASSERT(_cc_parse_url(&url, ipv6_url), "IPv6 URL parsing failed");
    ASSERT(url.ipv6, "IPv6 flag not set");
    ASSERT(_tcscmp(url.host, _T("2001:db8::1")) == 0, "IPv6 host mismatch");
    ASSERT(url.port == 8080, "Port mismatch");
    _cc_free_url(&url);

    return 0;
}

/* Test cases for URL encoding/decoding */
int test_url_encode() {
    tchar_t dst[256];
    const tchar_t *src = _T("Hello World!");
    int32_t result = _cc_url_encode(src, _tcslen(src), dst, 256);

    ASSERT(result == _tcslen(_T("Hello+World%21")), "URL encode length mismatch");
    ASSERT(_tcscmp(dst, _T("Hello+World%21")) == 0, "URL encode result mismatch");

    return 0;
}

int test_url_decode() {
    tchar_t dst[256];
    const tchar_t *src = _T("Hello+World%21");
    int32_t result = _cc_url_decode(src, _tcslen(src), dst, 256);

    ASSERT(result == 12, "URL decode length mismatch");
    ASSERT(_tcscmp(dst, _T("Hello World!")) == 0, "URL decode result mismatch");

    return 0;
}

/* Test cases for error handling */
int test_parse_invalid_url() {
    _cc_url_t url;
    const tchar_t *invalid_url = _T("http://example.com:invalid");

    ASSERT(!_cc_parse_url(&url, invalid_url), "Invalid URL parsing should fail");

    return 0;
}

/* Main function to run tests */
int main() {
    int failed = 0;

    failed += test_parse_url_scheme();
    failed += test_parse_url_with_credentials();
    failed += test_parse_ipv6_url();
    failed += test_url_encode();
    failed += test_url_decode();
    failed += test_parse_invalid_url();

    if (failed == 0) {
        printf("All URL tests passed!\n");
    } else {
        printf("%d URL tests failed!\n", failed);
    }
    char_t *urls = "http://user:pass@www.domain.com/index.php?id=1&tid=2#top";
    _cc_url_t url;
    _cc_alloc_url(&url, urls);
    printf("Scheme: %s\nHost: %s\nPort: %d\nPath: %s\nRequest: %s\nQuery: %s\nFragment: %s\nUsername: %s\nPassword: %s\n",
           url.scheme.value, url.host, url.port, url.path, url.request, url.query, url.fragment, url.username, url.password);
    _cc_free_url(&url);
    return failed;
}