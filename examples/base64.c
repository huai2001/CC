#include <stdio.h>

#include <locale.h>
#include <libcc/base64.h>
#include <libcc/base16.h>
#include <libcc/base58.h>
#include <libcc/sha.h>
#include <libcc/url.h>
#include <libcc/buf.h>
#include <libcc/alloc.h>

void base58Address(const tchar_t *data) {
    size_t data_length = _tcslen(data);
    byte_t bytes[1024] = {0};
    tchar_t output[1024] = {0};

    _cc_sha256_t sha256;
    byte_t results[_CC_SHA256_DIGEST_LENGTH_];

    data_length = _cc_hex2bytes(data, data_length, bytes, data_length);

    _cc_sha256_init(&sha256, false);
    _cc_sha256_update(&sha256, (byte_t*)bytes, data_length);
    _cc_sha256_final(&sha256, results);
    
    _cc_sha256_init(&sha256, false);
    _cc_sha256_update(&sha256, results, _CC_SHA256_DIGEST_LENGTH_);
    _cc_sha256_final(&sha256, results);

    memcpy(&bytes[data_length], results, 4);

    data_length = _cc_base58_encode((byte_t*)bytes, data_length + 4, output, 1024);
    _tprintf(_T("T : %s\n"),output);
}

int main (int argc, char * const argv[]) {
	tchar_t *in = _T("编码ABCD#$%^&*base编码   ");
    int32_t len = (int32_t)_tcslen(in);
    int32_t outlen = 0;
	tchar_t *out = nullptr, *out2 = nullptr;

    tchar_t baseX_en[256];
    tchar_t baseX_de[256];
    
	setlocale(LC_ALL, "chs");
    SetConsoleOutputCP(65001);

    base58Address(_T("4105ab06be97f2360d6fd2ea811233410d02f9f477"));
    
    outlen = _cc_base16_encode((byte_t*)in, len * sizeof(tchar_t), baseX_en, 256);
    
    _tprintf(_T("base16_encode : %s - %d\n"),baseX_en, outlen);
    
    outlen = _cc_base16_decode(baseX_en, outlen, (byte_t*)baseX_de, 256);
    
    _tprintf(_T("base16_decode : %s - %d\n"),baseX_de, outlen);

    outlen = _cc_base58_encode((byte_t*)in, len * sizeof(tchar_t), baseX_en, 256);
    
    _tprintf(_T("base58_encode : %s - %d\n"),baseX_en, outlen);
    
    outlen = _cc_base58_decode(baseX_en, outlen, (byte_t*)baseX_de, 256);
    
    _tprintf(_T("base58_decode : %s - %d\n"),baseX_de, outlen);

	len = len*sizeof(tchar_t);
	outlen = _CC_BASE64_EN_LEN(len);
    out = (tchar_t*)_cc_calloc(outlen,sizeof(tchar_t));
    len = _cc_base64_encode((byte_t*)in,len, out, outlen);
    _tprintf(_T("base64_encode : %s \n"),out);
	outlen = _CC_BASE64_DE_LEN(len);
    out2 = (tchar_t*)_cc_calloc(outlen,sizeof(tchar_t));
    _cc_base64_decode(out, len, (byte_t*)out2, outlen);
    _tprintf(_T("base64_decode : %s \n"),out2);

    _cc_safe_free(out);
    _cc_safe_free(out2);

    system("pause");
    return 0;
}
