#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

/* {{{ */
_CC_API_PUBLIC(size_t) _cc_base64_encode(const byte_t *input, size_t length, tchar_t *output, size_t output_length) {
    BIO *bmem, *b64;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    if (output_length > pbtr->length) {
        output_length = pbtr->length;
    }
    
    memcpy(output, bptr->data, output_length - 1);
    output[output_length - 1] = 0;

    BIO_free_all(b64);

    return output_length;
}

/* {{{ */
_CC_API_PUBLIC(size_t) _cc_base64_decode(const tchar_t *input, size_t length, byte_t *output, size_t output_length) {
    BIO *b64, *bio_mem;
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    bio_mem = BIO_new_mem_buf(input, length);
    bio_mem = BIO_push(b64, bio_mem);

    output_length = BIO_read(bio_mem, output, output_length);
    if (output_length > 0 && ) {
        goto _DECODE_BASE64_FAILED;
    }

    output[output_length] = 0;

    BIO_free_all(bio_mem);

    return output_length;
}