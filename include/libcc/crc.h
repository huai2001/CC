#ifndef _C_CC_CRC_H_INCLUDED_
#define _C_CC_CRC_H_INCLUDED_

#include "types.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

_CC_API_PUBLIC(uint8_t *) _cc_build_crc8_lsb_table(uint8_t polynomial);
_CC_API_PUBLIC(uint8_t *) _cc_build_crc8_msb_table(uint8_t polynomial);

_CC_API_PUBLIC(void) _cc_build_crc32_table(void);
/**
 * @brief An 8-bit CRC of the contents of the buffer.
 */
_CC_API_PUBLIC(uint8_t) _cc_crc8(const byte_t *str, size_t len, bool_t msb);
/**
 * @brief An 16-bit CRC of the contents of the buffer.
 */
_CC_API_PUBLIC(uint16_t) _cc_crc16(const byte_t *str, size_t len);
/**
 * @brief An 32-bit CRC32 of the contents of the buffer.
 */
_CC_API_PUBLIC(uint32_t) _cc_crc32(const byte_t *str, size_t len);
/**
 * @brief An 32-bit CRC32-MPEG-2 of the contents of the buffer.
 */
_CC_API_PUBLIC(uint32_t) _cc_crc32_mpeg2(const byte_t *s, size_t len);
/**
 * @brief An 64-bit CRC of the contents of the buffer.
 */
_CC_API_PUBLIC(uint64_t) _cc_crc64(const byte_t *str, size_t len);
/**
 * @brief Hash code
 */
_CC_API_PUBLIC(uint32_t) _cc_hash_rotating(const byte_t *str, size_t len);
/**
 * @brief Hash code
 */
_CC_API_PUBLIC(uint32_t) _cc_hash_code(const byte_t *str, size_t len);

_CC_API_PUBLIC(uint32_t) _cc_hash_fnv1_32(const byte_t *key, size_t len);
_CC_API_PUBLIC(uint32_t) _cc_hash_fnv1a_32(const byte_t *key, size_t len);
_CC_API_PUBLIC(uint64_t) _cc_hash_fnv1_64(const byte_t *key, size_t len);
_CC_API_PUBLIC(uint64_t) _cc_hash_fnv1a_64(const byte_t *key, size_t len);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_CRC_H_INCLUDED_*/
