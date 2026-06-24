#ifndef _C_CC_TCHAR_H_INCLUDED_
#define _C_CC_TCHAR_H_INCLUDED_

/**
 * @file tchar.h
 * @brief Portable Unicode character handling for cross-platform development
 *
 * This header provides TCHAR abstraction layer that automatically switches between
 * ANSI (char) and Unicode (wchar_t) string handling based on _CC_UNICODE_ macro.
 *
 * Supported compilers:
 *   - Microsoft Visual C++ (MSVC)
 *   - GNU Compiler Collection (GCC)
 *   - Clang/LLVM
 *
 * Supported platforms:
 *   - Windows (UTF-16 wchar_t, 2 bytes)
 *   - macOS/Linux/Unix (UTF-32 wchar_t, 4 bytes)
 */

#include "os/compiler.h"

/* Include character type headers for non-MSVC compilers */
#ifndef __CC_MSVC__
    #include <ctype.h>
    #include <wctype.h>
#endif

/* Determine wchar_t size */
#if defined(__SIZEOF_WCHAR_T__)
    #define CC_SIZEOF_WCHAR_T __SIZEOF_WCHAR_T__
#elif defined(__CC_WINDOWS__)
    #define CC_SIZEOF_WCHAR_T 2
#else
    #define CC_SIZEOF_WCHAR_T 4
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef _TCHAR_H_
#define _TCHAR_H_
/* ================================================================
 * UNICODE MODE (_CC_UNICODE_ defined)
 * ================================================================ */
#ifdef _CC_UNICODE_

/* Include wide character support */
#include <wchar.h>

/* End-of-file constant for wide characters */
#define _TEOF WEOF

/* ------------------------------------------------------------
 * Input/Output Functions
 * ------------------------------------------------------------ */
#define _tprintf        wprintf
#define _ftprintf       fwprintf
#define _stprintf       swprintf
#define _vtprintf       vwprintf
#define _vftprintf      vfwprintf
#define _vstprintf      vswprintf

/* ------------------------------------------------------------
 * Formatted String Functions (with buffer size)
 * ------------------------------------------------------------ */
#if defined(__CC_MSVC__)
    #define _sntprintf      _snwprintf
    #define _vsntprintf     _vsnwprintf
#else
    #define _sntprintf      swprintf
    #define _vsntprintf     vswprintf
#endif

/* String formatting length calculation */
#define _vsctprintf     _vscwprintf

/* ------------------------------------------------------------
 * Input Functions
 * ------------------------------------------------------------ */
#define _tscanf         wscanf
#define _ftscanf        fwscanf
#define _stscanf        swscanf

/* ------------------------------------------------------------
 * Character I/O Functions
 * ------------------------------------------------------------ */
#define _fgettc         fgetwc
#define _fgettchar      _fgetwchar
#define _fgetts         fgetws
#define _fputtc         fputwc
#define _fputtchar      _fputwchar
#define _fputts         fputws
#define _gettc          getwc
#define _puttc          putwc
#define _puttchar       putwchar
#define _putts          _putws
#define _ungettc        ungetwc

/* ------------------------------------------------------------
 * String Functions
 * ------------------------------------------------------------ */
#define _tcstod         wcstod
#define _tcstol         wcstol
#define _tcstoll        wcstoll
#define _tcstoul        wcstoul
#define _tcscat         wcscat
#define _tcschr         wcschr
#define _tcsrchr        wcsrchr
#define _tcscmp         wcscmp
#define _tcscpy         wcscpy
#define _tcscspn        wcscspn
#define _tcslen         wcslen
#define _tcsncat        wcsncat
#define _tcsncmp        wcsncmp
#define _tcsncpy        wcsncpy
#define _tcspbrk        wcspbrk
#define _tcsspn         wcsspn
#define _tcsstr         wcsstr
#define _tcstok         wcstok
#define _tcsdup         wcsdup
#define _tcsxfrm        wcsxfrm
#define _tcscoll        wcscoll

/* ------------------------------------------------------------
 * Case Conversion Functions
 * ------------------------------------------------------------ */
#define _totupper       towupper
#define _totlower       towlower

/* ------------------------------------------------------------
 * String Comparison (Case-insensitive)
 * ------------------------------------------------------------ */
#define _tcsicmp        _wcsicmp
#define _tcsnicmp       _wcsnicmp
#define _tcsicoll       _wcsicoll

/* ------------------------------------------------------------
 * String Modification Functions
 * ------------------------------------------------------------ */
#define _tcsnset        _wcsnset
#define _tcsrev         _wcsrev
#define _tcsset         _wcsset
#define _tcslwr         _wcslwr
#define _tcsupr         _wcsupr

/* ------------------------------------------------------------
 * Conversion Functions
 * ------------------------------------------------------------ */
#define _itot           _itow
#define _ltot           _ltow
#define _ultot          _ultow
#define _ttoi           _wtoi
#define _ttol           _wtol
#define _ttof           _wtof

/* ------------------------------------------------------------
 * Character Classification Functions
 * ------------------------------------------------------------ */
#define _istalpha       iswalpha
#define _istupper       iswupper
#define _istlower       iswlower
#define _istdigit       iswdigit
#define _istxdigit      iswxdigit
#define _istspace       iswspace
#define _istpunct       iswpunct
#define _istalnum       iswalnum
#define _istprint       iswprint
#define _istgraph       iswgraph
#define _istcntrl       iswcntrl
#define _istascii       iswascii

/* ------------------------------------------------------------
 * Date/Time Functions
 * ------------------------------------------------------------ */
#define _tcsftime       wcsftime
#define _tasctime       _wasctime
#define _tctime         _wctime
#define _tstrdate       _wstrdate
#define _tstrtime       _wstrtime
/* ------------------------------------------------------------
 * String Pointer Manipulation Macros
 * ------------------------------------------------------------ */
#define _tcsdec         _wcsdec
#define _tcsinc         _wcsinc
#define _tcsnextc       _wcsnextc
#define _tcsninc        _wcsninc
#define _tcsspnp        _wcsspnp
#define _tcsnbcnt       _wcsncnt
#define _tcsnccnt       _wcsncnt

/* Macro implementations for pointer manipulation */
#define _wcsdec(_wcs1, _wcs2)    ((_wcs1) >= (_wcs2) ? NULL : (_wcs2) - 1)
#define _wcsinc(_wcs)            ((_wcs) + 1)
#define _wcsnextc(_wcs)          ((unsigned int)(*(_wcs)))
#define _wcsninc(_wcs, _inc)     ((_wcs) + (_inc))
#define _wcsncnt(_wcs, _cnt)     ((wcslen(_wcs) > _cnt) ? _cnt : wcslen(_wcs))
#define _wcsspnp(_wcs1, _wcs2)   ((*((_wcs1) + wcsspn(_wcs1, _wcs2))) ? ((_wcs1) + wcsspn(_wcs1, _wcs2)) : NULL)

/* ------------------------------------------------------------
 * 64-bit Conversion Functions
 * ------------------------------------------------------------ */
#if defined(__CC_MSVC__)
    #define _ttoi64(str)        _wtoi64(str)
    #define _i64tot(val, buf)   _i64tow(val, buf, 10)
    #define _ui64tot(val, buf)  _ui64tow(val, buf, 10)
#else
    /* POSIX platforms need to handle endptr parameter */
    #define _ttoi64(str)        wcstoll(str, NULL, 10)
    #define _i64tot(val, buf)   _i64tow(val, buf, 10)
    #define _ui64tot(val, buf)  _ui64tow(val, buf, 10)
#endif

/* ------------------------------------------------------------
 * String Collation Functions
 * ------------------------------------------------------------ */
#define _tcsnccoll      _wcsncoll
#define _tcsncoll       _wcsncoll
#define _tcsncicoll     _wcsnicoll
#define _tcsnicoll      _wcsnicoll
/* ------------------------------------------------------------
 * File I/O Functions
 * ------------------------------------------------------------ */
#if defined(__CC_WINDOWS__)
    #define _tfdopen       _wfdopen
    #define _tfopen        _wfopen
    #define _tfreopen      _wfreopen
    #define _tfsopen       _wfsopen
    #define _topen         _wopen
    #define _tremove       _wremove
    #define _trename       _wrename
    #define _tsopen        _wsopen
    #define _tunlink       _wunlink
    #define _taccess       _waccess
    #define _tchmod        _wchmod
    #define _tcreat        _wcreat
#else
    /* POSIX/Unix platforms */
    #define _tfdopen       fdopen
    #define _tfopen        fopen
    #define _tfreopen      freopen
    #define _topen         open
    #define _tremove       remove
    #define _trename       rename
    #define _tunlink       unlink
    #define _taccess       access
    #define _tchmod        chmod
    #define _tcreat        creat
#endif

/* ------------------------------------------------------------
 * File Attribute Functions (Windows only)
 * ------------------------------------------------------------ */
#if defined(__CC_MSVC__)
    #define _tfindfirst    _wfindfirst
    #define _tfindnext     _wfindnext
    #define _tfinddata_t   _wfinddata_t
    #if __MSVCRT_VERSION__ >= 0x0800
        #define _tfindfirst64   _wfindfirst64
        #define _tfindfirst32   _wfindfirst32
        #define _tfindnext64    _wfindnext64
        #define _tfindnext32    _wfindnext32
    #endif
#endif

/* ------------------------------------------------------------
 * File System Functions
 * ------------------------------------------------------------ */
#define _tchdir         _wchdir
#define _tgetcwd        _wgetcwd
#define _tgetdcwd       _wgetdcwd
#define _tmkdir         _wmkdir
#define _trmdir         _wrmdir
#define _tutime         _wutime

/* ------------------------------------------------------------
 * File Status Functions
 * ------------------------------------------------------------ */
#ifdef __CC_WINDOWS__
    #define _tstat         _wstat
    #define _tstati64      _wstati64
    #define _tstat64       _wstat64
#else
    #define _tstat         stat
    #define _tstati64      stat64
    #define _tstat64       stat64
#endif

/* ------------------------------------------------------------
 * Environment Functions
 * ------------------------------------------------------------ */
#define _tgetenv        _wgetenv
#define _tputenv        _wputenv
#define _tsearchenv     _wsearchenv
#define _tsystem        _wsystem

/* ------------------------------------------------------------
 * Path Manipulation Functions
 * ------------------------------------------------------------ */
#define _tmakepath      _wmakepath
#define _tsplitpath     _wsplitpath
#define _tfullpath      _wfullpath

/* ------------------------------------------------------------
 * Memory Functions
 * ------------------------------------------------------------ */
#define _tmemchr        wmemchr
#define _tmktemp        _wmktemp
#define _tsetlocale     _wsetlocale
 
/* ------------------------------------------------------------
 * Directory Functions (Windows only)
 * ------------------------------------------------------------ */
#if defined(__CC_WINDOWS__)
    #define _tdirent      _wdirent
    #define _TDIR         _WDIR
    #define _topendir     _wopendir
    #define _tclosedir    _wclosedir
    #define _treaddir     _wreaddir
    #define _trewinddir   _wrewinddir
    #define _ttelldir     _wtelldir
    #define _tseekdir     _wseekdir
#endif

#else /* !_CC_UNICODE_ - ANSI/UTF-8 MODE */
 
/* ================================================================
 * ANSI/UTF-8 MODE (!_CC_UNICODE_)
 * ================================================================ */

/* End-of-file constant */
#define _TEOF EOF

/* ------------------------------------------------------------
 * Input/Output Functions
 * ------------------------------------------------------------ */
#define _tprintf        printf
#define _ftprintf       fprintf
#define _stprintf       sprintf
#define _vtprintf       vprintf
#define _vftprintf      vfprintf
#define _vstprintf      vsprintf
#define _vsctprintf     _vscprintf

/* Formatted string functions with buffer size */
#if defined(__CC_MSVC__)
    #define _sntprintf      _snprintf
    #define _vsntprintf     _vsnprintf
#else
    #define _sntprintf      snprintf
    #define _vsntprintf     vsnprintf
#endif

/* ------------------------------------------------------------
 * Input Functions
 * ------------------------------------------------------------ */
#define _tscanf         scanf
#define _ftscanf        fscanf
#define _stscanf        sscanf

/* ------------------------------------------------------------
 * Character I/O Functions
 * ------------------------------------------------------------ */
#define _fgettc         fgetc
#define _fgettchar      _fgetchar
#define _fgetts         fgets
#define _fputtc         fputc
#define _fputtchar      _fputchar
#define _fputts         fputs
#define _gettc          getc
#define _puttc          putc
#define _puttchar       putchar
#define _putts          puts
#define _ungettc        ungetc

/* ------------------------------------------------------------
 * String Functions
 * ------------------------------------------------------------ */
#define _tcstod         strtod
#define _tcstol         strtol
#define _tcstoll        strtoll
#define _tcstoul        strtoul
#define _tcscat         strcat
#define _tcschr         strchr
#define _tcsrchr        strrchr
#define _tcscmp         strcmp
#define _tcscpy         strcpy
#define _tcscspn        strcspn
#define _tcslen         strlen
#define _tcsncat        strncat
#define _tcsncmp        strncmp
#define _tcsncpy        strncpy
#define _tcspbrk        strpbrk
#define _tcsspn         strspn
#define _tcsstr         strstr
#define _tcstok         strtok
#define _tcsdup         strdup
#define _tcsxfrm        strxfrm
#define _tcscoll        strcoll

/* ------------------------------------------------------------
 * Case Conversion Functions
 * ------------------------------------------------------------ */
#define _totupper       toupper
#define _totlower       tolower

/* ------------------------------------------------------------
 * String Comparison (Case-insensitive)
 * ------------------------------------------------------------ */
#define _tcsicmp        _stricmp
#define _tcsnicmp       _strnicmp
#define _tcsicoll       _stricoll

/* ------------------------------------------------------------
 * String Modification Functions
 * ------------------------------------------------------------ */
#define _tcsnset        _strnset
#define _tcsrev         _strrev
#define _tcsset         _strset
#define _tcslwr         _strlwr
#define _tcsupr         _strupr

/* ------------------------------------------------------------
 * Conversion Functions
 * ------------------------------------------------------------ */
#define _itot           _itoa
#define _ltot           _ltoa
#define _ultot          _ultoa
#define _ttoi           atoi
#define _ttol           atol
#define _ttof           atof

/* ------------------------------------------------------------
 * Character Classification Functions
 * ------------------------------------------------------------ */
#define _istalpha       isalpha
#define _istupper       isupper
#define _istlower       islower
#define _istdigit       isdigit
#define _istxdigit      isxdigit
#define _istspace       isspace
#define _istpunct       ispunct
#define _istalnum       isalnum
#define _istprint       isprint
#define _istgraph       isgraph
#define _istcntrl       iscntrl
#define _istascii       isascii

/* ------------------------------------------------------------
 * Date/Time Functions
 * ------------------------------------------------------------ */
#define _tcsftime       strftime
#define _tasctime       asctime
#define _tctime         ctime
#define _tstrdate       _strdate
#define _tstrtime       _strtime
#define _tutime         _utime
 
/* ------------------------------------------------------------
 * String Pointer Manipulation Macros
 * ------------------------------------------------------------ */
#define _tcsdec         _strdec
#define _tcsinc         _strinc
#define _tcsnextc       _strnextc
#define _tcsninc        _strninc
#define _tcsspnp        _strspnp
#define _tcsnbcnt       _strncnt
#define _tcsnccnt       _strncnt

/* Macro implementations for pointer manipulation */
#define _strdec(_str1, _str2)    ((_str1) >= (_str2) ? NULL : (_str2) - 1)
#define _strinc(_str)            ((_str) + 1)
#define _strnextc(_str)          ((unsigned int)(*(_str)))
#define _strninc(_str, _inc)     ((_str) + (_inc))
#define _strncnt(_str, _cnt)     ((strlen(_str) > _cnt) ? _cnt : strlen(_str))
#define _strspnp(_str1, _str2)   ((*((_str1) + strspn(_str1, _str2))) ? ((_str1) + strspn(_str1, _str2)) : NULL)

/* ------------------------------------------------------------
 * 64-bit Conversion Functions
 * ------------------------------------------------------------ */
#if defined(__CC_MSVC__)
    #define _ttoi64(str)        _atoi64(str)
    #define _i64tot(val, buf)   _i64toa(val, buf, 10)
    #define _ui64tot(val, buf)  _ui64toa(val, buf, 10)
#else
    /* POSIX platforms need to handle endptr parameter */
    #define _ttoi64(str)        strtoll(str, NULL, 10)
    #define _i64tot(val, buf)   _i64toa(val, buf, 10)
    #define _ui64tot(val, buf)  _ui64toa(val, buf, 10)
#endif

/* ------------------------------------------------------------
 * String Collation Functions
 * ------------------------------------------------------------ */
#define _tcsnccoll      _strncoll
#define _tcsncoll       _strncoll
#define _tcsncicoll     _strnicoll
#define _tcsnicoll      _strnicoll

/* ------------------------------------------------------------
 * File I/O Functions
 * ------------------------------------------------------------ */
#if defined(__CC_WINDOWS__)
    #define _tfdopen       fdopen
    #define _tfopen        fopen
    #define _tfreopen      freopen
    #define _tfsopen       _fsopen
    #define _topen         open
    #define _tchmod        _chmod
    #define _tcreat        _creat
    #define _tremove       remove
    #define _trename       rename
    #define _tsopen        _sopen
    #define _tunlink       unlink
    #define _taccess       _access
#else
    #define _tfdopen       fdopen
    #define _tfopen        fopen
    #define _tfreopen      freopen
    #define _topen         open
    #define _tcreat        creat
    #define _tchmod        chmod
    #define _tremove       remove
    #define _trename       rename
    #define _tunlink       unlink
    #define _taccess       access
#endif

/* ------------------------------------------------------------
 * File Attribute Functions (Windows only)
 * ------------------------------------------------------------ */
#if defined(__CC_MSVC__)
    #define _tfindfirst    _findfirst
    #define _tfindnext     _findnext
    #define _tfinddata_t   _finddata_t
    #if __MSVCRT_VERSION__ >= 0x0800
        #define _tfindfirst64   _findfirst64
        #define _tfindfirst32   _findfirst32
        #define _tfindnext64    _findnext64
        #define _tfindnext32    _findnext32
    #endif
#endif

/* ------------------------------------------------------------
 * File System Functions
 * ------------------------------------------------------------ */
#define _tchdir         chdir
#define _tgetcwd        _getcwd
#define _tgetdcwd       _getdcwd
#if defined(__CC_WINDOWS__) || defined(_CC_UNICODE_)
    #define _tmkdir     mkdir
    #define _trmdir     rmdir
#else
    /* POSIX mkdir requires mode parameter, create wrapper for single argument */
    #define _tmkdir(path) mkdir(path, 0755)
    #define _trmdir     rmdir
#endif

/* ------------------------------------------------------------
 * File Status Functions
 * ------------------------------------------------------------ */
#ifdef __CC_WINDOWS__
    #define _tstat         _stat
    #define _tstati64      _stati64
    #define _tstat64       _stat64
#else
    #define _tstat         stat
    #define _tstati64      stat64
    #define _tstat64       stat64
#endif

/* ------------------------------------------------------------
 * Environment Functions
 * ------------------------------------------------------------ */
#define _tgetenv        getenv
#define _tputenv        putenv
#define _tsearchenv     _searchenv
#define _tsystem        system

/* ------------------------------------------------------------
 * Path Manipulation Functions
 * ------------------------------------------------------------ */
#define _tmakepath      _makepath
#define _tsplitpath     _splitpath
#define _tfullpath      _fullpath

/* ------------------------------------------------------------
 * Memory Functions
 * ------------------------------------------------------------ */
#define _tmemchr        memchr
#define _tmktemp        _mktemp
#define _tsetlocale     setlocale
 
/* ------------------------------------------------------------
 * Directory Functions
 * ------------------------------------------------------------ */
#if defined(__CC_WINDOWS__)
    #define _tdirent      dirent
    #define _TDIR         DIR
    #define _topendir     opendir
    #define _tclosedir    closedir
    #define _treaddir     readdir
    #define _trewinddir   rewinddir
    #define _ttelldir     telldir
    #define _tseekdir     seekdir
#endif

#endif /* !_CC_UNICODE_ */

/* ================================================================
 * String Literal Macros
 * ================================================================
 *
 * Use _T() or _TEXT() around string literals to automatically convert
 * between char* and wchar_t* based on _CC_UNICODE_ setting.
 *
 * Example:
 *   const tchar_t* str = _T("Hello World");
 *   - In ANSI mode:   becomes const char* str = "Hello World";
 *   - In Unicode mode: becomes const wchar_t* str = L"Hello World";
 * ================================================================ */
#ifndef _T
#define _T(x) x
#endif /*_T*/

#endif /* !_TCHAR_H_ */

/* ================================================================
 * Main Function Definition
 * ================================================================ */
#undef _tmain
#ifdef _CC_UNICODE_
    #define _tmain   wmain
#else
    #define _tmain   main
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_TCHAR_H_INCLUDED_ */
