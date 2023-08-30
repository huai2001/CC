#include <stdio.h>

#include <locale.h>
#include <cc/base64.h>
#include <cc/base16.h>
#include <cc/url.h>
#include <cc/alloc.h>


int main (int argc, char * const argv[]) {
	tchar_t *in = _T("ABCD#$%^&*ÖÐÎÄºº×Ö");
    int32_t len = (int32_t)_tcslen(in);
    int32_t outlen = 0;
	tchar_t *out = NULL, *out2 = NULL;

    tchar_t base16_en[256];
    tchar_t base16_de[256];
    
	setlocale(LC_ALL, "chs");
    
    
    outlen = _cc_base16_encode((byte_t*)in, len * sizeof(tchar_t), base16_en, 256);
    
    _tprintf(_T("base16_encode : %s - %d\n"),base16_en, outlen);
    
    outlen = _cc_base16_decode(base16_en, outlen, (byte_t*)base16_de, 256);
    
    _tprintf(_T("base16_decode : %s - %d\n"),base16_de, outlen);

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

    return 0;
}
