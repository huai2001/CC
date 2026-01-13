#include <libcc/time.h>
#include <libcc/crypto/hmac.h>
#include <libcc/crypto/base32.h>

/* HOTP - HMAC-Based One-Time Password (RFC 4226) */
static uint32_t hotp(_cc_hasher_t* hmac, uint64_t counter) {
    uint8_t digest[_CC_SHA1_DIGEST_LENGTH_];
    int32_t digest_length = _CC_SHA1_DIGEST_LENGTH_;
    uint8_t counter_bytes[8];
    int offset;
    uint32_t code;
    int i;

    /* Convert counter to big-endian bytes */
    for (i = 7; i >= 0; i--) {
        counter_bytes[i] = (uint8_t)(counter & 0xFF);
        counter >>= 8;
    }
    
    /* Compute HMAC-SHA1 */
    hmac->update(hmac, counter_bytes, 8);
    hmac->final(hmac, digest, &digest_length);
    
    /* Dynamic truncation */
    offset = digest[_CC_SHA1_DIGEST_LENGTH_ - 1] & 0x0F;
    code = ((uint32_t)(digest[offset] & 0x7F) << 24) |
           ((uint32_t)(digest[offset + 1] & 0xFF) << 16) |
           ((uint32_t)(digest[offset + 2] & 0xFF) << 8) |
           (uint32_t)(digest[offset + 3] & 0xFF);
    return code % 1000000; /* 6-digit code */
}

/* TOTP - Time-Based One-Time Password (RFC 6238) */
_CC_FORCE_INLINE_ uint64_t get_time_step(uint32_t time_step_seconds) {
    return (uint64_t)(time(NULL) / time_step_seconds);
}

_CC_API_PUBLIC(uint32_t) _cc_generate_totp(const tchar_t *secret, uint32_t time_step_seconds) {
    uint8_t password[64]; /* Max secret length */
    size_t length;
    uint64_t time_step;
    uint32_t code;
    _cc_hasher_t hmac;
    
    if (_cc_unlikely(secret == NULL || time_step_seconds == 0)) {
        return 0;
    }
    
    /* Decode Base32 secret */
    length = _cc_base32_decode(secret, _tcslen(secret), password, sizeof(password));
    if (length == 0) {
        return 0;
    }
    
    /* Get current time step */
    time_step = get_time_step(time_step_seconds);
    
    _cc_hmac_init(&hmac, _CC_SHA1_, password, length);
    /* Generate HOTP using time as counter */
    code = hotp(&hmac, time_step);
    hmac.free(&hmac);

    return code;
}

/* Verify TOTP code with a window of allowed steps */
_CC_API_PUBLIC(bool_t) _cc_verify_totp(const tchar_t *secret, uint32_t code, uint32_t time_step_seconds, int window) {
    uint8_t password[64];
    size_t length;
    uint64_t time_step;
    _cc_hasher_t hmac;
    int i;
    
    if (_cc_unlikely(secret == NULL || time_step_seconds == 0 || window < 0)) {
        return false;
    }

    /* Decode Base32 secret */
    length = _cc_base32_decode(secret, _tcslen(secret), password, sizeof(password));
    if (length == 0) {
        return false;
    }
    
    time_step = get_time_step(time_step_seconds);
    
    /* Check code in the allowed time window */
    _cc_hmac_init(&hmac, _CC_SHA1_, password, length);
    for (i = -window; i <= window; i++) {
        if (hotp(&hmac, time_step + (uint64_t)i) == code) {
            hmac.free(&hmac);
            return true;
        }
        hmac.reset(&hmac);
    }
    hmac.free(&hmac);
    return false;
}

/* Generate a random Base32 secret key */
_CC_API_PUBLIC(void) _cc_generate_secret(tchar_t *secret, size_t length) {
    const tchar_t charset[] = _T("ABCDEFGHIJKLMNOPQRSTUVWXYZ234567");
    size_t i;
    
    if (_cc_unlikely(secret == NULL || length == 0)) {
        return ;
    }

    for (i = 0; i < length - 1; i++) {
        secret[i] = charset[_cc_rand((sizeof(charset) - 1))];
    }

    secret[length - 1] = 0;
}