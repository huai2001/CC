/*
 * Copyright .Qiu<huai2011@163.com>. and other libCC contributors.
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
#ifndef _C_CC_LOGGER_H_INCLUDED_
#define _C_CC_LOGGER_H_INCLUDED_

#include "core.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
    
#define _CC_LOGGER_FLAGS_NORMAL_         0x0001
#define _CC_LOGGER_FLAGS_DEBUG_          0x0002
#define _CC_LOGGER_FLAGS_ERROR_          0x0004
#define _CC_LOGGER_FLAGS_WARNING_        0x0008
#define _CC_LOGGER_FLAGS_INFO_           0x0010

#define _CC_LOGGER_FLAGS_ASIC_           0x1000
#define _CC_LOGGER_FLAGS_UTF8_           0x2000
#define _CC_LOGGER_FLAGS_UTF16_          0x4000
/**/
#ifndef _L
    #define _L(x) __L(x)
    #define __L(x) L##x
#endif

typedef void (*_cc_loggerA_callback_t)(uint16_t flags, const char_t *log, size_t len, pvoid_t userdata);
typedef void (*_cc_loggerW_callback_t)(uint16_t flags, const wchar_t *log, size_t len, pvoid_t userdata);

/**/
_CC_API_PUBLIC(void)
_cc_loggerA_set_output_callback(_cc_loggerA_callback_t callback, pvoid_t userdata);
/**/
_CC_API_PUBLIC(void)
_cc_loggerW_set_output_callback(_cc_loggerW_callback_t callback, pvoid_t userdata);
/**/
_CC_API_PUBLIC(void)
_cc_logger_set_output_callback(_cc_loggerA_callback_t callbackA, _cc_loggerW_callback_t callbackW, pvoid_t userdata);

/**/
_CC_API_PUBLIC(void)
_cc_loggerA_vformat(uint16_t flags, const char_t* fmt, va_list arg);
/**/
_CC_API_PUBLIC(void) _cc_loggerA_format(uint16_t flags, const char_t* fmt, ...);
/**/
_CC_API_PUBLIC(void) _cc_loggerA(uint16_t flags, const char_t* str);
/**/
_CC_API_PUBLIC(void)
_cc_loggerW_format_args(uint16_t flags, const wchar_t* fmt, va_list arg);
/**/
_CC_API_PUBLIC(void) _cc_loggerW_format(uint16_t flags, const wchar_t* fmt, ...);
/**/
_CC_API_PUBLIC(void) _cc_loggerW(uint16_t flags, const wchar_t* str);

#ifdef _CC_MSVC_
#define _cc_logger_debugW(FMT, ...) \
    _cc_loggerW_format(_CC_LOGGER_FLAGS_DEBUG_, FMT, ##__VA_ARGS__)
#define _cc_logger_debugA(FMT, ...) \
    _cc_loggerA_format(_CC_LOGGER_FLAGS_DEBUG_, FMT, ##__VA_ARGS__)
#define _cc_logger_warinW(FMT, ...) \
    _cc_loggerW_format(_CC_LOGGER_FLAGS_WARNING_, FMT, ##__VA_ARGS__)
#define _cc_logger_warinA(FMT, ...) \
    _cc_loggerA_format(_CC_LOGGER_FLAGS_WARNING_, FMT, ##__VA_ARGS__)
#define _cc_logger_errorW(FMT, ...)                                \
    _cc_loggerW_format(_CC_LOGGER_FLAGS_ERROR_, L"[%s(%d)%s]" FMT, \
                       _L(_CC_FILE_), _CC_LINE_, _L(_CC_FUNC_), ##__VA_ARGS__)
#define _cc_logger_errorA(FMT, ...)                                          \
    _cc_loggerA_format(_CC_LOGGER_FLAGS_ERROR_, "[%s(%d)%s]" FMT, _CC_FILE_, \
                       _CC_LINE_, _CC_FUNC_, ##__VA_ARGS__)
#else
#define _cc_logger_debugW(FMT, ARGS...) \
    _cc_loggerW_format(_CC_LOGGER_FLAGS_DEBUG_, FMT, ##ARGS)
#define _cc_logger_debugA(FMT, ARGS...) \
    _cc_loggerA_format(_CC_LOGGER_FLAGS_DEBUG_, FMT, ##ARGS)
#define _cc_logger_warinW(FMT, ARGS...) \
    _cc_loggerW_format(_CC_LOGGER_FLAGS_WARNING_, FMT, ##ARGS)
#define _cc_logger_warinA(FMT, ARGS...) \
    _cc_loggerA_format(_CC_LOGGER_FLAGS_WARNING_, FMT, ##ARGS)
#define _cc_logger_errorW(FMT, ARGS...)                            \
    _cc_loggerW_format(_CC_LOGGER_FLAGS_ERROR_, L"[%s(%d)%s]" FMT, \
                       _L(_CC_FILE_), _CC_LINE_, _L(_CC_FUNC_), ##ARGS)
#define _cc_logger_errorA(FMT, ARGS...)                                      \
    _cc_loggerA_format(_CC_LOGGER_FLAGS_ERROR_, "[%s(%d)%s]" FMT, _CC_FILE_, \
                       _CC_LINE_, _CC_FUNC_, ##ARGS)
#endif

/**/
#ifdef _CC_UNICODE_
#define _cc_logger_format _cc_loggerW_format
#define _cc_logger _cc_loggerW
#define _cc_logger_warin _cc_logger_warinW
#define _cc_logger_debug _cc_logger_debugW
#define _cc_logger_error _cc_logger_errorW
#else
#define _cc_logger_format _cc_loggerA_format
#define _cc_logger _cc_loggerA
#define _cc_logger_warin _cc_logger_warinA
#define _cc_logger_debug _cc_logger_debugA
#define _cc_logger_error _cc_logger_errorA
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_LOGGER_H_INCLUDED_*/
