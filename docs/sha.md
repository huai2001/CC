# SHA1, SHA256, SHA512

> SHA1, SHA256, SHA512

SHA 宏
---------------

| 宏 | 描述  |
| :--------------- |:----------------------- |
| [_CC_SHA1_DIGEST_LENGTH_](#SHA1) | 编码数据长度 |
| [_CC_SHA224_DIGEST_LENGTH_](#SHA256) | 编码数据长度 |
| [_CC_SHA256_DIGEST_LENGTH_](#SHA256) | 编码数据长度 |
| [_CC_SHA384_DIGEST_LENGTH_](#SHA512) | 编码数据长度 |
| [_CC_SHA512_DIGEST_LENGTH_](#SHA512) | 编码数据长度 |

SHA 函数
---------------

| 函数 | 描述  |
| :--------------- |:----------------------- |
| [_cc_sha1_init](#SHA1) |  |
| [_cc_sha1_update](#SHA1) |  |
| [_cc_sha1_final](#SHA1) |  |
| [_cc_sha1](#SHA1) | 字符串SHA1 |
| [_cc_sha1file](#SHA1_file) | 文件SHA1 |
| [_cc_sha256_init](#SHA256) |  |
| [_cc_sha256_update](#SHA256) |  |
| [_cc_sha256_final](#SHA256) |  |
| [_cc_sha256](#SHA256) | 字符串SHA256 |
| [_cc_sha256file](#MD256_file) | 文件SHA256 |
| [_cc_sha512_init](#SHA512) |  |
| [_cc_sha512_update](#SHA512) |  |
| [_cc_sha512_final](#SHA512) |  |
| [_cc_sha512](#SHA512) | 字符串SHA256|
| [_cc_sha512file](#MSHA512_file) | 文件SHA256 |


## SHA1
```c
const char_t T_HEX_STRING[] = _T("0123456789ABCDEF");

byte_t t = 0;
int32_t i = 0, index = 0;
_cc_sha1_t c;
byte_t results[_CC_SHA1_DIGEST_LENGTH_];
char_t output[_CC_SHA1_DIGEST_LENGTH_ * 2];

char_t *input = "123456";
int32_t length = strlen(input);

_cc_sha1_init(&c);
_cc_sha1_update(&c,input, length);
_cc_sha1_final(&c, results);

for (i = 0; i < CC_SHA1_DIGEST_LENGTH; i++) {
    t = md[i];
    output[index++] = T_HEX_STRING[t / 16];
    output[index++] = T_HEX_STRING[t % 16];
}
output[index] = 0;

printf("SHA1:%s\n", output);

_cc_sha1(input, output);
printf("SHA1:%s\n", output);

```

## SHA1_file
```c
char_t output[_CC_SHA1_DIGEST_LENGTH_ * 2];

_cc_sha1file("filename.txt", output);

printf("SHAR1:%s\n", output);
```

## SHA256
```c
const char_t T_HEX_STRING[] = _T("0123456789ABCDEF");

bool_t is224 = false;

byte_t t = 0;
int32_t i = 0, index = 0;
_cc_sha256_t c;
byte_t results[_CC_SHA256_DIGEST_LENGTH_];
char_t output[_CC_SHA256_DIGEST_LENGTH_ * 2];
int32_t digest_length = is224 ? CC_SHA224_DIGEST_LENGTH : CC_SHA256_DIGEST_LENGTH;

char_t *input = "123456";
int32_t length = strlen(input);

_cc_sha256_init(&c);
_cc_sha256_update(&c,input, length);
_cc_sha256_final(&c, &(results[0]));

for (i = 0; i < digest_length; i++) {
    t = results[i];
    output[index++] = T_HEX_STRING[t / 16];
    output[index++] = T_HEX_STRING[t % 16];
}
output[index] = 0;

printf("SHA256:%s\n", output);

_cc_sha256(input, output, is224);
printf("SHA256:%s\n", output);
```
## SHA256_file
```c
char_t output[_CC_MD4_DIGEST_LENGTH_ * 2];

_cc_sha256file("filename.txt", output, false);

printf("SHA256:%s\n", output);
```

## SHA512
```c
const char_t T_HEX_STRING[] = _T("0123456789ABCDEF");

bool_t is384 = false;

byte_t t = 0;
int32_t i = 0, index = 0;
_cc_sha512_t c;
byte_t results[_CC_SHA512_DIGEST_LENGTH_];
char_t output[_CC_SHA512_DIGEST_LENGTH_ * 2];
int32_t digest_length = is384 ? CC_SHA384_DIGEST_LENGTH : CC_SHA512_DIGEST_LENGTH;

char_t *input = "123456";
int32_t length = strlen(input);

_cc_sha512_init(&c);
_cc_sha512_update(&c,input, length);
_cc_sha512_final(&c, &(results[0]));

for (i = 0; i < digest_length; i++) {
    t = results[i];
    output[index++] = T_HEX_STRING[t / 16];
    output[index++] = T_HEX_STRING[t % 16];
}
output[index] = 0;

printf("SHA512:%s\n", output);

_cc_sha512(input, output, is384);
printf("SHA512:%s\n", output);
```

## SHA512_file
```c
char_t output[_CC_MD5_DIGEST_LENGTH_ * 2];

_cc_sha512file("filename.txt", output, false);

printf("SHA512:%s\n", output);
```