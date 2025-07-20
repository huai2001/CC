/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
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
#ifndef _C_CC_BASE16_H_INCLUDED_
#define _C_CC_BASE16_H_INCLUDED_

#include "core.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
/** Compute size of needed storage for encoding. This function computes the
 *  exact size of a memory area needed to hold the result of an encoding
 *  operation, not including the terminating nullptr character.
 */
#define _CC_BASE16_EN_LEN(x) (x * 2)
/*
 * Compute size of needed storage for decoding. This function computes the
 *  estimated size of a memory area needed to hold the result of a decoding
 *  operation, not including the terminating nullptr character. Note that this
 *  function may return up to two bytes more due to the nature of Base64.
 */
#define _CC_BASE16_DE_LEN(x) ((x / 2))
/**
 * @brief     Encode a buffer into base16 format
 *
 * @param input   buffer holding the data
 * @param length   length of the input data
 * @param output   destination buffer
 * @param output_length   amount of data to be encoded
 *
 * @return    Encode a buffer into base16 format.
 */
_CC_API_PUBLIC(size_t)
_cc_base16_encode(const byte_t *input, size_t length, tchar_t *output, size_t output_length);
/**
 * @brief     Decode a base16-formatted buffer
 *
 * @param input   buffer holding the  data
 * @param length   length of the input data
 * @param output   destination buffer
 * @param output_length   amount of data to be decoded
 *
 * @return    Decode a base16-formatted buffer.
 */
_CC_API_PUBLIC(size_t)
_cc_base16_decode(const tchar_t *input, size_t length, byte_t *output, size_t output_length);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_BASE16_H_INCLUDED_ */
