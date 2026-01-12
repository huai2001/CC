/**
 * Google Authenticator (TOTP/HOTP) Implementation in C
 * 
 * Implementation based on RFC 4226 (HOTP) and RFC 6238 (TOTP)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

/* HMAC-SHA1 Implementation */
#define SHA1_DIGEST_SIZE 20
#define SHA1_BLOCK_SIZE 64

typedef struct {
    uint32_t h[5];
    uint8_t buffer[64];
    uint32_t total_len;
} SHA1_CTX;

static const uint32_t SHA1_K[80] = {
    0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6
};

#define ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

static void sha1_transform(SHA1_CTX *ctx) {
    uint32_t W[80], t;
    uint8_t *p = ctx->buffer;
    int i;

    for (i = 0; i < 16; i++) {
        W[i] = (p[i * 4] << 24) | (p[i * 4 + 1] << 16) | 
               (p[i * 4 + 2] << 8) | p[i * 4 + 3];
    }

    for (i = 16; i < 80; i++) {
        W[i] = ROTL32(W[i - 3] ^ W[i - 8] ^ W[i - 14] ^ W[i - 16], 1);
    }

    for (i = 0; i < 5; i++) {
        ctx->h[i] = ctx->h[i];
    }

    for (i = 0; i < 80; i++) {
        uint32_t f, k, temp;
        
        if (i < 20) {
            f = (ctx->h[1] & ctx->h[2]) | (~ctx->h[1] & ctx->h[3]);
            k = SHA1_K[0];
        } else if (i < 40) {
            f = ctx->h[1] ^ ctx->h[2] ^ ctx->h[3];
            k = SHA1_K[1];
        } else if (i < 60) {
            f = (ctx->h[1] & ctx->h[2]) | (ctx->h[1] & ctx->h[3]) | (ctx->h[2] & ctx->h[3]);
            k = SHA1_K[2];
        } else {
            f = ctx->h[1] ^ ctx->h[2] ^ ctx->h[3];
            k = SHA1_K[3];
        }

        temp = ROTL32(ctx->h[0], 5) + f + ctx->h[4] + W[i] + k;
        ctx->h[4] = ctx->h[3];
        ctx->h[3] = ctx->h[2];
        ctx->h[2] = ROTL32(ctx->h[1], 30);
        ctx->h[1] = ctx->h[0];
        ctx->h[0] = temp;
    }
}

static void sha1_init(SHA1_CTX *ctx) {
    ctx->h[0] = 0x67452301;
    ctx->h[1] = 0xEFCDAB89;
    ctx->h[2] = 0x98BADCFE;
    ctx->h[3] = 0x10325476;
    ctx->h[4] = 0xC3D2E1F0;
    ctx->total_len = 0;
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
}

static void sha1_update(SHA1_CTX *ctx, const uint8_t *data, size_t len) {
    size_t i;
    
    for (i = 0; i < len; i++) {
        ctx->buffer[ctx->total_len % SHA1_BLOCK_SIZE] = data[i];
        ctx->total_len++;
        
        if (ctx->total_len % SHA1_BLOCK_SIZE == 0) {
            sha1_transform(ctx);
        }
    }
}

static void sha1_final(SHA1_CTX *ctx, uint8_t *digest) {
    uint64_t bit_len = ctx->total_len * 8;
    size_t pad_len = SHA1_BLOCK_SIZE - ((ctx->total_len + 9) % SHA1_BLOCK_SIZE);
    int i;
    
    ctx->buffer[ctx->total_len % SHA1_BLOCK_SIZE] = 0x80;
    
    for (i = 1; i <= pad_len; i++) {
        ctx->buffer[(ctx->total_len + i) % SHA1_BLOCK_SIZE] = 0;
    }
    
    /* Store the bit length at the end */
    for (i = 0; i < 8; i++) {
        ctx->buffer[SHA1_BLOCK_SIZE - 8 + i] = (bit_len >> (56 - i * 8)) & 0xFF;
    }
    
    sha1_transform(ctx);
    
    for (i = 0; i < SHA1_DIGEST_SIZE; i++) {
        digest[i] = (ctx->h[i / 4] >> (24 - (i % 4) * 8)) & 0xFF;
    }
}

static void hmac_sha1(const uint8_t *key, size_t key_len,
                      const uint8_t *data, size_t data_len,
                      uint8_t *digest) {
    SHA1_CTX ctx;
    uint8_t k_ipad[SHA1_BLOCK_SIZE], k_opad[SHA1_BLOCK_SIZE];
    uint8_t temp_digest[SHA1_DIGEST_SIZE];
    int i;
    
    memset(k_ipad, 0x36, sizeof(k_ipad));
    memset(k_opad, 0x5C, sizeof(k_opad));
    
    if (key_len > SHA1_BLOCK_SIZE) {
        SHA1_CTX temp_ctx;
        sha1_init(&temp_ctx);
        sha1_update(&temp_ctx, key, key_len);
        sha1_final(&temp_ctx, temp_digest);
        for (i = 0; i < SHA1_DIGEST_SIZE; i++) {
            k_ipad[i] ^= temp_digest[i];
            k_opad[i] ^= temp_digest[i];
        }
    } else {
        for (i = 0; i < key_len; i++) {
            k_ipad[i] ^= key[i];
            k_opad[i] ^= key[i];
        }
    }
    
    sha1_init(&ctx);
    sha1_update(&ctx, k_ipad, SHA1_BLOCK_SIZE);
    sha1_update(&ctx, data, data_len);
    sha1_final(&ctx, temp_digest);
    
    sha1_init(&ctx);
    sha1_update(&ctx, k_opad, SHA1_BLOCK_SIZE);
    sha1_update(&ctx, temp_digest, SHA1_DIGEST_SIZE);
    sha1_final(&ctx, digest);
}

/* Base32 Encoding/Decoding for Google Authenticator secret keys */
static const char BASE32_ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
static const int BASE32_DECODE_TABLE[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 26, 27, 28, 29, 30, 31, -1, -1, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

static size_t base32_decode(const char *encoded, uint8_t *output, size_t max_len) {
    size_t i = 0, buffer = 0, bits = 0, out_len = 0;
    
    while (encoded[i] != '\0' && out_len < max_len) {
        int val = BASE32_DECODE_TABLE[(unsigned char)encoded[i]];
        
        if (val >= 0) {
            buffer = (buffer << 5) | val;
            bits += 5;
            
            if (bits >= 8) {
                output[out_len++] = (buffer >> (bits - 8)) & 0xFF;
                bits -= 8;
            }
        }
        i++;
    }
    
    return out_len;
}

/* HOTP - HMAC-Based One-Time Password (RFC 4226) */
static uint32_t hotp(const uint8_t *key, size_t key_len, uint64_t counter) {
    uint8_t digest[SHA1_DIGEST_SIZE];
    uint8_t counter_bytes[8];
    int offset, i;
    uint32_t code;
    
    /* Convert counter to big-endian bytes */
    for (i = 7; i >= 0; i--) {
        counter_bytes[i] = counter & 0xFF;
        counter >>= 8;
    }
    
    /* Compute HMAC-SHA1 */
    hmac_sha1(key, key_len, counter_bytes, 8, digest);
    
    /* Dynamic truncation */
    offset = digest[SHA1_DIGEST_SIZE - 1] & 0x0F;
    code = ((digest[offset] & 0x7F) << 24) |
           ((digest[offset + 1] & 0xFF) << 16) |
           ((digest[offset + 2] & 0xFF) << 8) |
           (digest[offset + 3] & 0xFF);
    
    return code % 1000000; /* 6-digit code */
}

/* TOTP - Time-Based One-Time Password (RFC 6238) */
static uint64_t get_time_step(uint32_t time_step_seconds) {
    return (uint64_t)(time(NULL) / time_step_seconds);
}

uint32_t generate_totp(const char *secret_base32, uint32_t time_step_seconds) {
    uint8_t key[64]; /* Max secret length */
    size_t key_len;
    uint64_t time_step;
    
    if (!secret_base32) {
        return 0;
    }
    
    /* Decode Base32 secret */
    key_len = base32_decode(secret_base32, key, sizeof(key));
    if (key_len == 0) {
        return 0;
    }
    
    /* Get current time step */
    time_step = get_time_step(time_step_seconds);
    
    /* Generate HOTP using time as counter */
    return hotp(key, key_len, time_step);
}

/* Verify TOTP code with a window of allowed steps */
int verify_totp(const char *secret_base32, uint32_t code, 
                uint32_t time_step_seconds, int window) {
    uint8_t key[64];
    size_t key_len;
    uint64_t time_step;
    int i;
    
    if (!secret_base32) {
        return 0;
    }
    
    /* Decode Base32 secret */
    key_len = base32_decode(secret_base32, key, sizeof(key));
    if (key_len == 0) {
        return 0;
    }
    
    time_step = get_time_step(time_step_seconds);
    
    /* Check code in the allowed time window */
    for (i = -window; i <= window; i++) {
        if (hotp(key, key_len, time_step + i) == code) {
            return 1;
        }
    }
    
    return 0;
}

/* Generate a random Base32 secret key */
void generate_secret(char *secret, size_t len) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    size_t i;
    
    for (i = 0; i < len - 1; i++) {
        secret[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    secret[len - 1] = '\0';
}

/* Display usage example */
void print_example(const char *secret) {
    uint32_t code;
    time_t now;
    uint32_t remaining;
    
    printf("Google Authenticator Demo\n");
    printf("=========================\n\n");
    printf("Secret Key (Base32): %s\n", secret);
    printf("QR Code URL (use with Google Authenticator app):\n");
    printf("otpauth://totp/Example:alice@google.com?secret=%s&issuer=Example\n\n", secret);
    
    printf("Current TOTP Codes:\n");
    printf("-------------------\n");
    
    for (int i = 0; i < 5; i++) {
        now = time(NULL);
        remaining = 30 - (now % 30);
        code = generate_totp(secret, 30);
        
        printf("Code: %06u | Expires in: %2u seconds\n", code, remaining);
        sleep(1);
    }
}

int main(int argc, char *argv[]) {
    char secret[17]; /* 16-character Base32 secret */
    
    /* Seed random number generator */
    srand((unsigned int)time(NULL));
    
    if (argc > 1) {
        /* Use provided secret */
        strncpy(secret, argv[1], sizeof(secret) - 1);
        secret[sizeof(secret) - 1] = '\0';
    } else {
        /* Generate random secret */
        generate_secret(secret, sizeof(secret));
    }
    
    print_example(secret);
    
    return 0;
}
