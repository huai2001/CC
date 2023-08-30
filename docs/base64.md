# Base64

> 一个简单的Base64例子

## Base64.c file
```cpp
//返回值：编码后长度
int32_t _cc_base64_encode(
                        const byte_t *input,    //参数1：input 明文字符串
                        int32_t length,         //参数2：length 明文字符串长度
                        byte_t *output,         //参数3：output 编码后的base64字符串
                        int32_t output_length   //参数4：output_length 编码长度
                    );
//返回值：解码后长度
int32_t _cc_base64_decode(
                        const byte_t *input,    //参数1：input 编码字符串
                        int32_t length,         //参数2：length 编码字符串长度
                        byte_t *output,         //参数3：output 解码后的字符串
                        int32_t output_length   //参数4：output_length 明文长度
                    );
```

| 函数 | 描述  |
| :---------------:|:-----------------------:|
| CC_BASE64_EN_LEN | 计算Base64编码所需空间长度 |
| CC_BASE64_DE_LEN | 计算Base64解码所需空间长度 |
| _cc_base64_encode | 对字符串进行Base 编码 |
| _cc_base64_decode | 对字符串进行Base 解码 |


## 例子
```cpp
....
#include "cc/base64.h"

int main (int argc, char * const argv[])
{
    char_t *in = "中文＃¥@#%ABCDEF";
    int32_t len = _tcslen(in);
    int32_t out_base64_en_len = 0;
    int32_t out_base64_de_len = 0;
    uchar_t *out_base64_en = NULL;
    uchar_t *out_base64_de = NULL;

    out_base64_en = (uchar_t*)_cc_calloc(_CC_BASE64_EN_LEN(len),sizeof(uchar_t));
    out_base64_en_len = _cc_base64_encode((byte_t*)in, len, out_base64_en, out_base64_en_len);
    printf("base64_encode : %s\n",out_base64_en);

    out_base64_de = (uchar_t*)_cc_calloc(_CC_BASE64_DE_LEN(out_base64_en_len),sizeof(uchar_t));
    out_base64_de_len = _cc_base64_decode((byte_t*)out_base64_en, out_base64_en_len, out_base64_de, len);
    printf("base64_decode : %s \n",out_base64_de);

    _cc_free(out_base64_en);
    _cc_free(out_base64_de);
    return 1;
}
```