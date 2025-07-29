#include <stdio.h>
#include <libcc.h>
#include <locale.h>

void url_decode_and_encode() {
	tchar_t *testes = _T("abcd@#$%^&*()");
	tchar_t testes_en[1024];
	tchar_t testes_de[1024];
	int32_t testes_len = 1024;
	testes_len = _cc_raw_url_encode(testes, (int32_t)_tcslen(testes), testes_en, 1024);
    //%E6%96%87abc
	_tprintf(_T("url_raw_encode: %s - %d\n"), testes_en, testes_len);

	testes_len = _cc_raw_url_decode(testes_en, testes_len, testes_de, 1024);

	_tprintf(_T("url_raw_decode: %s\n"), testes_de);


	testes_len = _cc_url_encode(testes, (int32_t)_tcslen(testes), testes_en, 1024);

	_tprintf(_T("url_encode: %s - %d\n"), testes_en, testes_len);

	testes_len = _cc_url_decode(testes_en, testes_len, testes_de, 1024);

	_tprintf(_T("url_decode: %s\n"), testes_de);
}


int main (int argc, char * const argv[]) {   //
    tchar_t *str = _T("http://usernameabc%u4e2dB%u56fd:123654789.com@[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]/rest/160601/ocr/ocr_idcard.json?uid=100&fid=1#post");
    _cc_url_t url;

	//setlocale(LC_ALL, "chs");

    if (_cc_alloc_url(&url, str)) {
        _tprintf(_T("scheme:%s\n"),url.scheme.value);
        _tprintf(_T("host:%s\n"),url.host);
        _tprintf(_T("port:%d\n"),url.port);
        _tprintf(_T("path:%s\n"),url.path);
        /* URL Components (URL-Quoted)*/
        _tprintf(_T("request:%s\n"),url.request);
        _tprintf(_T("query:%s\n"),url.query);
        _tprintf(_T("fragment:%s\n"),url.fragment);
        _tprintf(_T("username:%s\n"),url.username);
        _tprintf(_T("password:%s\n"),url.password);
    }
    
    _cc_free_url(&url);
    str = _T("http://usernameabc%u4e2dB%u56fd:dsa%3a%2f%2f%26%3ff%23%25%24%5e553@rm-m5e3c7r12h9rnbqd2jo.mysql.rds.aliyuncs.com:8080");
    if (_cc_alloc_url(&url, str)) {
        _tprintf(_T("scheme:%s\n"),url.scheme.value);
        _tprintf(_T("host:%s\n"),url.host);
        _tprintf(_T("port:%d\n"),url.port);
        _tprintf(_T("path:%s\n"),url.path);
        /* URL Components (URL-Quoted)*/
        _tprintf(_T("request:%s\n"),url.request);
        _tprintf(_T("query:%s\n"),url.query);
        _tprintf(_T("fragment:%s\n"),url.fragment);
        _tprintf(_T("username:%s\n"),url.username);
        _tprintf(_T("password:%s\n"),url.password);
    }
    
    _cc_free_url(&url);

    //url_decode_and_encode();
    return 0;
}
