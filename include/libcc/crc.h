/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
 * All rights reserved.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:

 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/
#ifndef _C_CC_CRC_H_INCLUDED_
#define _C_CC_CRC_H_INCLUDED_

#include "types.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

_CC_API_PUBLIC(uint8_t *) _cc_build_crc8_lsb_table(uint8_t polynomial);
_CC_API_PUBLIC(uint8_t *) _cc_build_crc8_msb_table(uint8_t polynomial);

void _cc_build_crc32_table(void);
/**
 * @brief An 8-bit CRC of the contents of the buffer.
 *
 * @param str data
 * @param len Size of data
 * @param msb POLYNOMIAL
 *
 * @return 8-bit CRC
 */
_CC_API_PUBLIC(uint8_t) _cc_crc8(const byte_t *str, size_t len, bool_t msb);
/**
 * @brief An 16-bit CRC of the contents of the buffer.
 *
 * @param str data
 * @param len Size of data
 *
 * @return 16-bit CRC
 */
_CC_API_PUBLIC(uint16_t) _cc_crc16(const byte_t *str, size_t len);
/**
 * @brief An 32-bit CRC32 of the contents of the buffer.
 *
 * @param str data
 * @param len Size of data
 *
 * @return 32-bit CRC
 */
_CC_API_PUBLIC(uint32_t) _cc_crc32(const byte_t *str, size_t len);

/**
 * @brief An 32-bit CRC32-MPEG-2 of the contents of the buffer.
 *
 * @param str data
 * @param len Size of data
 *
 * @return 32-bit CRC
 */
_CC_API_PUBLIC(uint32_t) _cc_crc32_mpeg2(const byte_t *s, size_t len);
/**
 * @brief An 64-bit CRC of the contents of the buffer.
 *
 * @param str data
 * @param len Size of data
 *
 * @return 64-bit CRC
 */
_CC_API_PUBLIC(uint64_t) _cc_crc64(const byte_t *str, size_t len);
/**
 * @brief Hash code
 *
 * @param str data
 * @param len Size of data
 *
 * @return rotating hash code
 */
_CC_API_PUBLIC(uint32_t) _cc_hash_rotating(const byte_t *str, size_t len);
/**
 * @brief Hash code
 *
 * @param str data
 * @param len Size of data
 *
 * @return hash code
 */
_CC_API_PUBLIC(uint32_t) _cc_hash(const byte_t *str, size_t len);

_CC_API_PUBLIC(uint32_t) _cc_hash_fnv1_32(const byte_t *key, size_t len);
_CC_API_PUBLIC(uint32_t) _cc_hash_fnv1a_32(const byte_t *key, size_t len);
_CC_API_PUBLIC(uint64_t) _cc_hash_fnv1_64(const byte_t *key, size_t len);
_CC_API_PUBLIC(uint64_t) _cc_hash_fnv1a_64(const byte_t *key, size_t len);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_CRC_H_INCLUDED_*/
