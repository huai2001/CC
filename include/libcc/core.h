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
#ifndef _C_CC_CORE_H_INCLUDED_
#define _C_CC_CORE_H_INCLUDED_

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "types.h"

/* _taccess include */
#ifdef __CC_WINDOWS__
#include  <io.h>
#else
#include <unistd.h>
#endif
 
#ifdef __CC_WINDOWS__
    #include "core/windows.h"
#elif __CC_ANDROID__
    #include "core/linux.h"
    #include "core/android.h"
#elif __CC_LINUX__
    #include "core/linux.h"
#else
    #include "core/unix.h"
#endif

#include "core/file.h"
#include "core/cpu_info.h"
#include "rand.h"

#if (defined(DEBUG) | defined(_DEBUG) | defined(NDK_DEBUG)) 
    #define _CC_DEBUG_
    #undef NDEBUG
    #ifndef __CC_ANDROID__
        #include <assert.h>
        #define _cc_assert assert
    #else
        /**
         * assert() equivalent.
         */
        #define _cc_assert(expr) \
        if (!(expr)) { \
            __android_log_print(ANDROID_LOG_ERROR, _CC_ANDROID_TAG_, "Assertion failed: %s (%s: %d)\n", #expr, __FILE__, __LINE__); \
            abort(); \
        }
    #endif
#else
    #define _cc_assert(expr) ((void)0)
    #undef _CC_DEBUG_
#endif

#define _cc_abort() do {\
    _cc_loggerA_format(_CC_LOGGER_FLAGS_ERROR_, "[%s(%d)%s] abort"_CC_FILE_, _CC_LINE_, _CC_FUNC_);\
    fflush(stderr);\
    abort();\
} while (0)

/* Should be safe for any weird systems that do not define it */
#ifndef _CC_MAX_PATH_
    #define _CC_MAX_PATH_ (256)
#endif

#define _CC_1K_BUFFER_SIZE_     (1024)
#define _CC_2K_BUFFER_SIZE_     (1 << 11)
#define _CC_4K_BUFFER_SIZE_     (1 << 12)
#define _CC_8K_BUFFER_SIZE_     (1 << 13)

#define _CC_16K_BUFFER_SIZE_    (1 << 14)
#define _CC_32K_BUFFER_SIZE_    (1 << 15)
#define _CC_64K_BUFFER_SIZE_    (1 << 16)

#ifndef _CC_IO_BUFFER_SIZE_
#define _CC_IO_BUFFER_SIZE_     _CC_16K_BUFFER_SIZE_
#endif

/*CR: Carriage Return*/
#define _CC_CR_ '\r'
/*LF: Linefeed*/
#define _CC_LF_ '\n'
/*CRLF: Carriage Return & Linefeed*/
#define _CC_CRLF_ "\r\n"

#ifdef __CC_WINDOWS__
    #define _CC_SLASH_C_ '\\'
    #define _CC_SLASH_S_ "\\"
#else
    #define _CC_SLASH_C_ '/'
    #define _CC_SLASH_S_ "/"
#endif

#define _CC_IS_SLASH(x) ((x) == _T('\\') || (x) == _T('/'))
 
#define _CC_UNUSED(_x) ( (void)(_x) )

/**
 * Count the number of elements in an array. The array must be defined
 * as such; using this with a dynamically allocated array will give
 * incorrect results.
 * For example:
   static char a[] = "foo";     -- _countof(a) == 4 (note terminating \0)
   int a[5] = {1, 2};           -- _countof(a) == 5
   char *a[] = {                -- _countof(a) == 3
     "foo", "bar", "baz"
   }; */
#ifndef _countof
#define _countof(_x) (sizeof(_x)/sizeof((_x)[0]))
#endif
    
#define _cc_countof _countof
/**
 *   Calculate the address of the base of the structure given its type, and an
 *   address of a field within the structure.
 */
#ifndef _cc_upcast

#define _cc_upcast(_address, _type, _member) \
    (_type *)((char *)_address - ((size_t) &((_type *)0)->_member))
/*
#define _cc_upcast(_address, _type, _member) ({ \
    const typeof( ((_type *)0)->_member ) *__mptr = (_address); \
    (_type *)( (char *)__mptr - ((size_t) &((_type *)0)->_member) );\
})
*/
#endif

/*
 * msvc and icc7 compile memset() to the inline "rep stos"
 * while ZeroMemory() and bzero() are the calls.
 * icc7 may also inline several mov's of a zeroed register for small blocks.
 */
#ifndef bzero
#define bzero(_a, _s) memset((_a), 0, (_s))
#endif

/**/
#define _CC_CHECKING_BIT(_number, _x) ((((_number) >> (_x)) & 1) == 1)
/**/
#define _CC_SET_BIT(_needle, _flags) ((_flags) |= (_needle))
#define _CC_UNSET_BIT(_needle, _flags) ((_flags) &= (~(_needle)))
#define _CC_ISSET_BIT(_needle, _flags) ((_flags) & (_needle))
#define _CC_MODIFY_BIT(_needle, _removed, _flags) \
do { \
    (_flags) &= (~_removed);\
    (_flags) |= (_needle);\
} while ( 0 )

/**/
#define _CC_BUILD_INT16(_l, _h) ((uint16_t)(((byte_t)((uint32_t)(_l) & 0xff)) | ((uint16_t)((byte_t)((uint32_t)(_h) & 0xff))) << 8))
#define _CC_BUILD_INT32(_l, _h) ((uint32_t)(((uint16_t)((uint32_t)(_l) & 0xffff)) | ((uint32_t)((uint16_t)((uint32_t)(_h) & 0xffff))) << 16))
#define _CC_LO_UINT16(_l)       ((uint16_t)((uint32_t)(_l) & 0xffff))
#define _CC_HI_UINT16(_h)       ((uint16_t)(((uint32_t)(_h) >> 16) & 0xffff))
#define _CC_LO_UINT8(_l)        ((uint8_t)((uint32_t)(_l) & 0xff))
#define _CC_HI_UINT8(_h)        ((uint8_t)(((uint32_t)(_h) >> 8) & 0xff))

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_ISDIGIT(V)   ((unsigned)(V) - '0' < 10u)
#define _CC_ISLOWER(V)   ((unsigned)(V) - 'a' < 26u)
#define _CC_ISUPPER(V)   ((unsigned)(V) - 'A' < 26u)
#define _CC_ISALPHA(V)   (_CC_ISLOWER(V) || _CC_ISUPPER(V))
#define _CC_ISXDIGIT(V)  (_CC_ISDIGIT(V) || ((unsigned)(V|0x20) - 'a' < 6u))

/*(' ') 0x20    space (SPC)             */
/*('\t')0x09    horizontal tab (TAB)    */
/*('\n')0x0a    newline (LF)            */
/*('\v')0x0b    vertical tab (VT)       */
/*('\f')0x0c    feed (FF)               */
/*('\r')0x0d    carriage return (CR)    */
#define _CC_ISSPACE(V) ((unsigned)((V) - 0x09) < 5u || (V) == 0x20)

_CC_FORCE_INLINE_ bool_t _cc_isdigit(int c) {
    return _CC_ISDIGIT(c);
}

_CC_FORCE_INLINE_ bool_t _cc_isxdigit(int c) {
    return _CC_ISXDIGIT(c);
}

_CC_FORCE_INLINE_ bool_t _cc_isalpha(int c) {
    return _CC_ISALPHA(c);
}

_CC_FORCE_INLINE_ bool_t _cc_isalnum(int c) {
    return _CC_ISDIGIT(c) || _CC_ISALPHA(c);
}

_CC_FORCE_INLINE_ bool_t _cc_isspace(int c) {
    return _CC_ISSPACE(c);
}

_CC_FORCE_INLINE_ size_t _cc_aligned_alloc_opt(size_t number, size_t alignment) {
    const size_t mask = alignment - 1;
    if (_cc_builtin_constant(alignment)) {
        return (number + mask) & ~mask;
    }
    _cc_assert((alignment & mask) == 0);
    return (number + mask) & ~mask;
}

#ifdef _CC_MSVC_

#ifndef va_copy
    #if defined (__GNUC__) && defined (__PPC__) && (defined (_CALL_SYSV) || defined (_WIN32))
        #define va_copy(dst, src) (*(dst) = *(src))
    #else
        #define va_copy(dst, src) ((dst) = (src))
    #endif
#endif /* va_copy */

#endif /* _CC_MSVC_ */

/**/
#ifdef __CC_WIN32_CE__
    /*-- Called from fileio.c */
    _CC_API_PUBLIC(int) unlink(const tchar_t *);
    #if __CC_WIN32_CE__ < 211
    _CC_API_PUBLIC(int) fflush(FILE *);
    #endif
#endif

/**
 *
 */
_CC_API_PUBLIC(size_t) _cc_nextpow2(size_t);
/**
 *
 */
_CC_API_PUBLIC(void) _cc_get_preferred_languages(tchar_t *buf, size_t buflen);

/**
 *
 */
_CC_API_PUBLIC(size_t) _cc_fpath(tchar_t *buf, size_t size, const tchar_t *fmt, ...);

/**
 *
 */
_CC_API_PUBLIC(void) _cc_replace_slashes(tchar_t* path);
/**
 * @brief mkdir
 *
 * @param path
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_mkdir(const tchar_t *path);
/**
 * @brief mkdir
 *
 * @param path
 * @param is_dir true: Full Directory, false: file
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_create_directory(const tchar_t *path, bool_t is_dir);

typedef enum _cc_folder {
    _CC_FOLDER_HOME_,        /**< The folder which contains all of the current user's data, preferences, and documents. It usually contains most of the other folders. If a requested folder does not exist, the home folder can be considered a safe fallback to store a user's documents. */
    _CC_FOLDER_DESKTOP_,     /**< The folder of files that are displayed on the desktop. Note that the existence of a desktop folder does not guarantee that the system does show icons on its desktop; certain GNU/Linux distros with a graphical environment may not have desktop icons. */
    _CC_FOLDER_DOCUMENTS_,   /**< User document files, possibly application-specific. This is a good place to save a user's projects. */
    _CC_FOLDER_DOWNLOADS_,   /**< Standard folder for user files downloaded from the internet. */
    _CC_FOLDER_MUSIC_,       /**< Music files that can be played using a standard music player (mp3, ogg...). */
    _CC_FOLDER_PICTURES_,    /**< Image files that can be displayed using a standard viewer (png, jpg...). */
    _CC_FOLDER_PUBLICSHARE_, /**< Files that are meant to be shared with other users on the same computer. */
    _CC_FOLDER_SAVEDGAMES_,  /**< Save files for games. */
    _CC_FOLDER_SCREENSHOTS_, /**< Application screenshots. */
    _CC_FOLDER_TEMPLATES_,   /**< Template files to be used when the user requests the desktop environment to create a new file in a certain folder, such as "New Text File.txt".  Any file in the Templates folder can be used as a starting point for a new file. */
    _CC_FOLDER_VIDEOS_,      /**< Video files that can be played using a standard video player (mp4, webm...). */
    _CC_FOLDER_COUNT_        /**< Total number of types in this enum, not a folder type by itself. */
} _cc_folder_t;


_CC_API_PUBLIC(size_t) _cc_get_executable_path(tchar_t *path, size_t len);
_CC_API_PUBLIC(size_t) _cc_get_base_path(tchar_t *path, size_t len);
_CC_API_PUBLIC(size_t) _cc_get_folder(_cc_folder_t folder, tchar_t *path, size_t len);
_CC_API_PUBLIC(size_t) _cc_get_cwd(tchar_t *path, size_t len);
/**
 * @brief Get system errno
 *
 * @return 0 if successful or system on error.
 */
_CC_API_PUBLIC(int32_t) _cc_last_errno(void);
/**
 * @brief Set system errno
 *
 * @param _errno
 */
_CC_API_PUBLIC(void) _cc_set_last_errno(int32_t _errno);
/**
 * @brief Get system error message
 *
 * @param _errno
 *
 * @return System error message
 */
_CC_API_PUBLIC(tchar_t*) _cc_last_error(int32_t _errno);

/**
 * @brief Set clipboard text
 *
 * @param text
 *
 * @return 0
 */
_CC_API_PUBLIC(int32_t) _cc_set_clipboard_text(const tchar_t *text);
/**
 * @brief Get clipboard Text
 *
 * @param str String buffer
 * @param len Maximum length of string buffer
 *
 * @return Length of string
 */
_CC_API_PUBLIC(int32_t) _cc_get_clipboard_text(tchar_t *str, int32_t len);
/**
 * @brief Check the clipboard for strings
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_has_clipboard_text(void);

/**
 * @brief delete file
 *
 * @param file file path
 *
 * @return 0 if successful or system on error.
*/
#ifdef __CC_IPHONEOS__
    _CC_API_PUBLIC(int) _cc_ios_unlink(const tchar_t *file); 
    #define _cc_unlink(x) _cc_ios_unlink(x)
#else 
    #define _cc_unlink(x) _tunlink(x)
#endif /*__CC_IPHONEOS__*/

#ifdef __CC_ANDROID__
_CC_FORCE_INLINE_ bool_t _cc_open_url(const tchar_t *url) {
    return Android_JNI_OpenURL(url);
}
#else
_CC_API_PUBLIC(bool_t) _cc_open_url(const tchar_t *url);
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_CORE_H_INCLUDED_ */
