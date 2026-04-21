# AES 加密算法教程

## 文件概述
`aes.h` 是 libcc 项目中的 AES（高级加密标准）实现，提供高效的对称加密功能。该文件遵循开源许可证，支持多种加密模式和密钥长度。

## 主要功能

### 1. AES 加密/解密
-   **`_cc_aes_set_key`**：设置 AES 密钥，支持 128/192/256 位密钥。
-   **`_cc_aes_encrypt`**：AES 加密（ECB 模式）。
-   **`_cc_aes_decrypt`**：AES 解密（ECB 模式）。

### 2. 加密模式
-   **ECB（电子密码本模式）**：简单并行，但不安全（不推荐用于加密大量数据）。
-   **CBC（密码块链接模式）**：更安全，推荐使用。

### 3. 密钥长度
-   128 位（16 字节）
-   192 位（24 字节）
-   256 位（32 字节）

## 使用示例

### 示例 1：AES-256-CBC 加密
```c
#include <libcc.h>
#include <libcc/crypto/aes.h>
#include <stdio.h>
#include <string.h>

int main() {
    uint8_t key[32] = {0}; // 256-bit key
    uint8_t iv[16] = {0};  // Initialization vector
    uint8_t plaintext[16] = "Hello, AES!";
    uint8_t ciphertext[16];
    uint8_t decrypted[16];
    _cc_aes_ctx_t ctx;

    /* 设置密钥 */
    if (!_cc_aes_set_key(&ctx, key, 256)) {
        fprintf(stderr, "Failed to set AES key\n");
        return EXIT_FAILURE;
    }

    /* CBC 模式加密 */
    _cc_aes_cbc_encrypt(&ctx, iv, plaintext, ciphertext, 16);

    /* CBC 模式解密 */
    _cc_aes_cbc_decrypt(&ctx, iv, ciphertext, decrypted, 16);

    /* 验证结果 */
    if (memcmp(plaintext, decrypted, 16) == 0) {
        printf("AES encryption/decryption successful\n");
    } else {
        printf("AES encryption/decryption failed\n");
    }

    return EXIT_SUCCESS;
}
```

### 示例 2：AES-128-ECB 加密
```c
#include <libcc.h>
#include <libcc/crypto/aes.h>

int main() {
    uint8_t key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                       0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    uint8_t plaintext[16] = "Hello, World!";
    uint8_t ciphertext[16];
    uint8_t decrypted[16];
    _cc_aes_ctx_t ctx;

    /* 设置 128 位密钥 */
    _cc_aes_set_key(&ctx, key, 128);

    /* ECB 模式加密 */
    _cc_aes_encrypt(&ctx, plaintext, ciphertext);

    /* ECB 模式解密 */
    _cc_aes_decrypt(&ctx, ciphertext, decrypted);

    return EXIT_SUCCESS;
}
```

## 详细说明

### 数据结构
```c
typedef struct _cc_aes_ctx {
    uint32_t ek[60];  // 加密密钥调度
    uint32_t dk[60];  // 解密密钥调度
    int32_t nr;       // 轮数
} _cc_aes_ctx_t;
```

### 函数接口
-   **`bool_t _cc_aes_set_key(_cc_aes_ctx_t *ctx, const uint8_t *key, int32_t key_len)`**
  -   初始化 AES 上下文
  -   `key_len`：128、192 或 256
  -   返回：成功 true，失败 false

-   **`void _cc_aes_encrypt(const _cc_aes_ctx_t *ctx, const uint8_t *plaintext, uint8_t *ciphertext)`**
  -   ECB 模式加密单个块（16 字节）

-   **`void _cc_aes_decrypt(const _cc_aes_ctx_t *ctx, const uint8_t *ciphertext, uint8_t *plaintext)`**
  -   ECB 模式解密单个块（16 字节）

-   **`void _cc_aes_cbc_encrypt(_cc_aes_ctx_t *ctx, uint8_t *iv, const uint8_t *plaintext, uint8_t *ciphertext, size_t length)`**
  -   CBC 模式加密
  -   `iv`：初始化向量（16 字节），函数会修改 iv

-   **`void _cc_aes_cbc_decrypt(_cc_aes_ctx_t *ctx, uint8_t *iv, const uint8_t *ciphertext, uint8_t *plaintext, size_t length)`**
  -   CBC 模式解密
  -   `iv`：初始化向量（16 字节），函数会修改 iv

## 安全注意事项
1. **密钥管理**：使用强随机数生成密钥，确保密钥安全存储和传输。
2. **IV 使用**：CBC 模式下，每次加密使用不同的 IV，防止重放攻击。
3. **填充方案**：AES 要求数据长度为 16 字节倍数，使用 PKCS#7 等填充方案。
4. **模式选择**：优先使用 CBC 模式，ECB 模式仅用于兼容性。
5. **密钥长度**：推荐使用 256 位密钥以获得最高安全性。
6. **认证**：AES 仅提供机密性，考虑使用 HMAC 或 GCM 模式提供认证。

## 性能考虑
-   AES-NI 指令集支持：现代 CPU 支持硬件加速 AES 操作。
-   块大小：每次处理 16 字节，适合流式处理大数据。
-   内存使用：上下文结构约 480 字节，适合资源受限环境。

## 编译选项
确保在构建 libcc 时包含加密模块：
```
USE_LIB_OPENSSL=1  # 如果需要 OpenSSL 集成
```

## 许可证
该文件遵循开源许可证，详细信息请参考文件头部的注释。