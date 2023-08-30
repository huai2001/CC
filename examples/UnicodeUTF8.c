#include <stdio.h>
#include <cc/base64.h>
#include <cc/alloc.h>
#include <locale.h>

/*
 void testes_print(int32_t *data, int32_t len, NSStringEncoding encoding) {
     NSData *nsData = [NSData dataWithBytes:data length:len];
     NSString *str = [[NSString alloc] initWithData:nsData encoding:encoding];
     NSLog(@"%@", str);
 }
 */
int main (int argc, char * const argv[]) {
    char_t *testes ="Abc $ £¤¡¡ÖÐÎÄ zh_CN.UTF-8!";
    char_t utf8[1024] = {0};
    setlocale(LC_ALL, "chs");
    uint16_t utf16[1024] = {0};//{0x0041,0x0062,0x0063,0x0020,0x0024,0x0020,0xffe5,0x3000,0x4e2d,0x6587,0x0055,0x0054,0x0046,0x002d,0x0038,0x0021};
    uint32_t utf32[1024] = {0};
    _cc_utf8_to_utf32((uint8_t*)testes, (uint8_t*)(testes+strlen(testes)), (uint32_t*)utf32, (uint32_t*)(utf32+1024), false);
    //testes_print((wchar_t*)utf32,NSUTF32LittleEndianStringEncoding);
    _cc_utf8_to_utf16((uint8_t*)testes, (uint8_t*)(testes+strlen(testes)), (uint16_t*)utf16, (uint16_t*)(utf16+1024), false);
    //testes_print((int32_t*)utf61,NSUTF16LittleEndianStringEncoding);
    
    
    _cc_utf16_to_utf8((uint16_t*)utf16, (uint16_t*)(utf16+_cc_countof(utf16)), (uint8_t*)utf8, (uint8_t*)(utf8+1024), false);
    
    printf("%s,%ld\n", utf8, sizeof(wchar_t));
    return 0;
}
