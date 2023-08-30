# AES

> AES 加密算法

AES 宏
---------------

| 宏 | 描述  |
| :--------------- |:----------------------- |
| [_CC_AES_ENCRYPT](#AES_ENC) | 编码数据长度 |
| [_CC_AES_DECRYPT](#AES_DEC) | 编码数据长度 |
| [_CC_AES_BUFFER_LEN](#AES) | 编码数据长度 |

AES 函数
---------------

| 函数 | 描述  |
| :--------------- |:----------------------- |
| [_cc_aes_init](#AES_ENC) | 初始化 |
| [_cc_aes_setkey_enc](#AES_ENC) | 设置编码密钥 |
| [_cc_aes_setkey_dec](#AES_DEC) | 设置解码密码 |
| [_cc_aes_setkey](#AES) | 设置密钥 |
| [_cc_aes_crypt_ecb](#AES) |  |
| [_cc_aes_crypt_cbc](#AES) |  |
| [_cc_aes_crypt_cfb128](#AES) |  |
| [_cc_aes_crypt_cfb8](#AES) |  |
| [_cc_aes_crypt_ctr](#AES) | |
| [_cc_aes_encrypt](#AES_ENC) | 对数据进行编码 |
| [_cc_aes_decrypt](#AES_DEC) | 对数据进行解码 |


## AES_ENC
```c
struct testes{
    int a;
};

static byte_t key[32] = {
    0x58, 0x0a, 0x06, 0xe9, 0x97, 0x07, 0x59, 0x5c,
    0x9e, 0x19, 0xd2, 0xa7, 0xbb, 0x40, 0x2b, 0x7a,
    0xc7, 0xd8, 0x11, 0x9e, 0x4c, 0x51, 0x35, 0x75,
    0x64, 0x28, 0x0f, 0x23, 0xad, 0x74, 0xac, 0x37
};

FILE *fw = NULL;
//编码对象
_cc_aes_t aes_enc;


byte_t encrypt_data[_CC_AES_BLOCK_SIZE]; 
byte_t data[_CC_AES_BUFFER_LEN(sizeof(struct testes))];

int32_t encrypt_len = 0;
byte_t file_version = 0;
bzero(data,sizeof(data));
bzero(encrypt_data,sizeof(encrypt_data));


fw = _tfopen("aes_testes.bin", _T("wb"));
if (fw == NULL) {
    CC_ERROR_LOG(_T("Could't open aes_testes.bin file."));
    return 0;
}

memcpy(data, &fconfig, sizeof(bd_config_t));

//编码对象
_cc_aes_init(&aes_enc);
_cc_aes_setkey_enc(&aes_enc, key, 128);

while(encrypt_len < sizeof(data)){
    _cc_aes_encrypt(&aes_enc, &data[encrypt_len],encrypt_data);
    fwrite(encrypt_data, sizeof(byte_t), CC_AES_BLOCK_SIZE, fw);
    encrypt_len += CC_AES_BLOCK_SIZE;
}

fclose(fw);

```

## AES_DEC
```c
struct testes{
    int a;
};

static byte_t key[32] = {
    0x58, 0x0a, 0x06, 0xe9, 0x97, 0x07, 0x59, 0x5c,
    0x9e, 0x19, 0xd2, 0xa7, 0xbb, 0x40, 0x2b, 0x7a,
    0xc7, 0xd8, 0x11, 0x9e, 0x4c, 0x51, 0x35, 0x75,
    0x64, 0x28, 0x0f, 0x23, 0xad, 0x74, 0xac, 0x37
};

byte_t decrypt_data[_CC_AES_BUFFER_LEN(sizeof(struct testes))]; 
byte_t data[_CC_AES_KEY_SIZE];
_cc_aes_t aes_dec;
int32_t decrypt_len = 0;
FILE* fr = _tfopen("aes_testes.bin", _T("rb"));

if(fr == NULL) {
    CC_ERROR_LOG(_T("Could't open aes_testes.bin file."));
    return false;
}

bzero(data,sizeof(data));
bzero(decrypt_data,sizeof(decrypt_data));

_cc_aes_init(&aes_dec);
_cc_aes_setkey_dec(&aes_dec, key, 128);

while(fread(data, sizeof(byte_t), CC_AES_BLOCK_SIZE, fr) > 0) {
    _cc_aes_decrypt(&aes_dec, data, &decrypt_data[decrypt_len]);
    decrypt_len += CC_AES_BLOCK_SIZE;
}
fclose(fr);

```