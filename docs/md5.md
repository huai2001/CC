# MD5，MD4，MD2

> MD5，MD4，MD2

MD2函数
---------------

| 函数 | 描述  |
| :--------------- |:----------------------- |
| [_cc_md2_init](#MD2) |  |
| [_cc_md2_update](#MD2) |  |
| [_cc_md2_final](#MD2) |  |
| [_CC_MD2_DIGEST_LENGTH__](#MD2) | 编码数据长度（宏） |
| [_cc_md2](#MD2) | 字符串MD2 |
| [_cc_md2file](#MD2_file) | 文件MD2 |
| [_cc_md2_fp](#MD2_file) | FILE*指针文件MD2 |

MD2函数
---------------

| 函数 | 描述  |
| :--------------- |:----------------------- |
| [_cc_md4_init](#MD4) |  |
| [_cc_md4_update](#MD4) |  |
| [_cc_md4_final](#MD4) |   |
| [_CC_MD4_DIGEST_LENGTH__](#MD4) | 编码数据长度（宏） |
| [_cc_md4](#MD4) | 字符串MD4 |
| [_cc_md4file](#MD5_file) | 文件MD4 |
| [_cc_md4_fp](#MD2_file) | FILE*指针文件MD4 |

MD5函数
---------------

| 函数 | 描述  |
| :--------------- |:----------------------- |
| [_cc_md5_init](#MD5) |  |
| [_cc_md5_update](#MD5) |  |
| [_cc_md5_final](#MD5) |   |
| [_CC_MD5_DIGEST_LENGTH__](#MD5) | 编码数据长度（宏） |
| [_cc_md5](#MD5) | 字符串MD5 |
| [_cc_md5file](#MD5_file) | 文件MD5 |
| [_cc_md5_fp](#MD2_file) | FILE*指针文件MD5 |


## MD2
```c
const char_t T_HEX_STRING[] = _T("0123456789ABCDEF");

byte_t t = 0;
int32_t i = 0, index = 0;
_cc_md2_t c;
byte_t md[_CC_MD2_DIGEST_LENGTH__];
char_t output[_CC_MD2_DIGEST_LENGTH__ * 2];

char_t *input = "123456";
int32_t length = strlen(input);

_cc_md2_init(&c);
_cc_md2_update(&c,input, length);
_cc_md2_final(&c, &(md[0]));

for (i = 0; i < _CC_MD2_DIGEST_LENGTH__; i++) {
    t = md[i];
    output[index++] = T_HEX_STRING[t / 16];
    output[index++] = T_HEX_STRING[t % 16];
}
output[index] = 0;

printf("MD2:%s\n", output);

_cc_md2(input, output);
printf("MD2:%s\n", output);

```

## MD2_file
```c
char_t output[_CC_MD2_DIGEST_LENGTH__ * 2];

_cc_md4file("filename.txt", output);

printf("MD2:%s\n", output);
```

## MD4
```c
const char_t T_HEX_STRING[] = _T("0123456789ABCDEF");

byte_t t = 0;
int32_t i = 0, index = 0;
_cc_md5_t c;
byte_t md[_CC_MD4_DIGEST_LENGTH__];
char_t output[_CC_MD4_DIGEST_LENGTH__ * 2];

char_t *input = "123456";
int32_t length = strlen(input);

_cc_md4_init(&c);
_cc_md4_update(&c,input, length);
_cc_md4_final(&c, &(md[0]));

for (i = 0; i < _CC_MD4_DIGEST_LENGTH__; i++) {
    t = md[i];
    output[index++] = T_HEX_STRING[t / 16];
    output[index++] = T_HEX_STRING[t % 16];
}
output[index] = 0;

printf("MD4:%s\n", output);

_cc_md4(input, output);
printf("MD4:%s\n", output);
```
## MD4_file
```c
char_t output[_CC_MD4_DIGEST_LENGTH__ * 2];

_cc_md4file("filename.txt", output);

printf("MD4:%s\n", output);
```

## MD5
```c
const char_t T_HEX_STRING[] = _T("0123456789ABCDEF");

byte_t t = 0;
int32_t i = 0, index = 0;
_cc_md5_t c;
byte_t md[_CC_MD5_DIGEST_LENGTH__];
char_t output[_CC_MD5_DIGEST_LENGTH__ * 2];

char_t *input = "123456";
int32_t length = strlen(input);

_cc_md5_init(&c);
_cc_md5_update(&c,input, length);
_cc_md5_final(&c, &(md[0]));

for (i = 0; i < _CC_MD5_DIGEST_LENGTH__; i++) {
    t = md[i];
    output[index++] = T_HEX_STRING[t / 16];
    output[index++] = T_HEX_STRING[t % 16];
}
output[index] = 0;

printf("MD5:%s\n", output);

_cc_md4(input, output);
printf("MD5:%s\n", output);
```

## MD5_file
```c
char_t output[_CC_MD5_DIGEST_LENGTH__ * 2];

_cc_md5file("filename.txt", output);

printf("MD5:%s\n", output);
```