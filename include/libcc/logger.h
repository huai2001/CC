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
#ifndef _C_CC_LOGGER_H_INCLUDED_
#define _C_CC_LOGGER_H_INCLUDED_

#include "syslog.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**/
_CC_API_PUBLIC(void) _cc_loggerA_vformat(uint8_t level, const char_t* fmt, va_list arg);
/**/
_CC_API_PUBLIC(void) _cc_loggerA_format(uint8_t level, const char_t* fmt, ...);
/**/
_CC_API_PUBLIC(void) _cc_loggerA(uint8_t level, const char_t* str);
/**/
_CC_API_PUBLIC(void) _cc_loggerW_vformat(uint8_t level, const wchar_t* fmt, va_list arg);
/**/
_CC_API_PUBLIC(void) _cc_loggerW_format(uint8_t level, const wchar_t* fmt, ...);
/**/
_CC_API_PUBLIC(void) _cc_loggerW(uint8_t level, const wchar_t* str);

#ifdef _CC_MSVC_
    /**/
    #define _cc_loggerW_debug(FMT, ...) \
        _cc_loggerW_format(_CC_LOG_LEVEL_DEBUG_, FMT, ##__VA_ARGS__)
    #define _cc_loggerA_debug(FMT, ...) \
        _cc_loggerA_format(_CC_LOG_LEVEL_DEBUG_, FMT, ##__VA_ARGS__)

    #define _cc_loggerW_info(FMT, ...) \
        _cc_loggerW_format(_CC_LOG_LEVEL_INFO_, FMT, ##__VA_ARGS__)
    #define _cc_loggerA_info(FMT, ...) \
        _cc_loggerA_format(_CC_LOG_LEVEL_INFO_, FMT, ##__VA_ARGS__)

    #define _cc_loggerW_warin(FMT, ...) \
        _cc_loggerW_format(_CC_LOG_LEVEL_WARNING_, FMT, ##__VA_ARGS__)
    #define _cc_loggerA_warin(FMT, ...) \
        _cc_loggerA_format(_CC_LOG_LEVEL_WARNING_, FMT, ##__VA_ARGS__)

    #define _cc_loggerW_error(FMT, ...) \
        _cc_loggerW_format(_CC_LOG_LEVEL_ERROR_, FMT, ##__VA_ARGS__)
    #define _cc_loggerA_error(FMT, ...) \
        _cc_loggerA_format(_CC_LOG_LEVEL_ERROR_, FMT, ##__VA_ARGS__)
#else
    #define _cc_loggerW_debug(FMT, ARGS...) \
        _cc_loggerW_format(_CC_LOG_LEVEL_DEBUG_, FMT, ##ARGS)
    #define _cc_loggerA_debug(FMT, ARGS...) \
        _cc_loggerA_format(_CC_LOG_LEVEL_DEBUG_, FMT, ##ARGS)

    #define _cc_loggerW_info(FMT, ARGS...) \
        _cc_loggerW_format(_CC_LOG_LEVEL_INFO_, FMT, ##ARGS)
    #define _cc_loggerA_info(FMT, ARGS...) \
        _cc_loggerA_format(_CC_LOG_LEVEL_INFO_, FMT, ##ARGS)

    #define _cc_loggerW_warin(FMT, ARGS...) \
        _cc_loggerW_format(_CC_LOG_LEVEL_WARNING_, FMT, ##ARGS)
    #define _cc_loggerA_warin(FMT, ARGS...) \
        _cc_loggerA_format(_CC_LOG_LEVEL_WARNING_, FMT, ##ARGS)

    #define _cc_loggerW_error(FMT, ARGS...) \
        _cc_loggerW_format(_CC_LOG_LEVEL_ERROR_, FMT, ##ARGS)
    #define _cc_loggerA_error(FMT, ARGS...) \
        _cc_loggerA_format(_CC_LOG_LEVEL_ERROR_, FMT, ##ARGS)
#endif

/**/
#ifdef _CC_UNICODE_
#define _cc_logger_format _cc_loggerW_format
#define _cc_logger _cc_loggerW
#define _cc_logger_warin _cc_loggerW_warin
#define _cc_logger_debug _cc_loggerW_debug
#define _cc_logger_info _cc_loggerW_info
#define _cc_logger_error _cc_loggerW_error
#define _cc_logger_syslog _cc_loggerW_syslog
#else
#define _cc_logger_format _cc_loggerA_format
#define _cc_logger _cc_loggerA
#define _cc_logger_warin _cc_loggerA_warin
#define _cc_logger_debug _cc_loggerA_debug
#define _cc_logger_info _cc_loggerA_info
#define _cc_logger_error _cc_loggerA_error
#define _cc_logger_syslog _cc_loggerA_syslog
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_LOGGER_H_INCLUDED_*/
