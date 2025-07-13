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
#ifndef _C_CC_TCHAR_H_INCLUDED_
#define _C_CC_TCHAR_H_INCLUDED_

#include "core/compiler.h"
 
#ifndef __CC_WINDOWS__
#include <ctype.h>
#include <wctype.h>
#endif

/* __AUTO_TCHAR_H_USESYS__ */
#undef __AUTO_TCHAR_H_USESYS__

#if defined(_CC_MSVC_) /* MSVC. */
    #define __AUTO_TCHAR_H_USESYS__
#elif defined(__BORLANDC__) /* BCB. */
    #define __AUTO_TCHAR_H_USESYS__
#elif defined(__CC_WINDOWS__) || defined(__CC_MINGW__)
    /**/
    #define __AUTO_TCHAR_H_USESYS__
#else
    /**/
#endif

#if defined(__SIZEOF_WCHAR_T__)
#define CC_SIZEOF_WCHAR_T __SIZEOF_WCHAR_T__
#elif defined(__CC_WINDOWS__)
#define CC_SIZEOF_WCHAR_T 2
#else  // assume everything else is UTF-32 (add more tests if compiler-assert fails below!)
#define CC_SIZEOF_WCHAR_T 4
#endif

/* __AUTO_TCHAR_H_USESYS__ */
#ifdef __AUTO_TCHAR_H_USESYS__
    #include <tchar.h>
#else

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
    
#ifndef _TCHAR_H_
#define _TCHAR_H_
/** ++++++++++++++++++++ UNICODE ++++++++++++++++++++ */
#ifdef _CC_UNICODE_
/*
* Include <wchar.h> for wchar_t and WEOF if _UNICODE.
*/
#include <wchar.h>
 
/*
* Use _TEOF instead of EOF or WEOF. It will be appropriately translated if
* _UNICODE is correctly defined (or not).
*/
#define _TEOF WEOF

/* for porting from other Windows compilers */
#if 0 /* no wide startup module */
#define _tmain wmain
#define _tWinMain wWinMain
#define _tenviron _wenviron
#define __targv __wargv
#endif
 /*
 * Unicode functions
 */
#define _tprintf wprintf
#define _ftprintf fwprintf
#define _stprintf swprintf
    
#define _sntprintf _snwprintf
#define _vsntprintf _vsnwprintf

#define _vtprintf vwprintf
#define _vftprintf vfwprintf
#define _vstprintf vswprintf
#define _vsctprintf _vscwprintf
    
#define _tscanf wscanf
#define _ftscanf fwscanf
#define _stscanf swscanf
#define _fgettc fgetwc
#define _fgettchar _fgetwchar
#define _fgetts fgetws
#define _fputtc fputwc
#define _fputtchar _fputwchar
#define _fputts fputws
#define _gettc getwc
#define _getts _getws
#define _puttc putwc
#define _puttchar putwchar
#define _putts _putws
#define _ungettc ungetwc
#define _tcstod wcstod
#define _tcstol wcstol
#define _tcstoul wcstoul
#define _itot _itow
#define _ltot _ltow
#define _ultot _ultow
#define _ttoi _wtoi
#define _ttol _wtol
#define _ttof _wtof
#define _tcscat wcscat
#define _tcschr wcschr
#define _tcscmp wcscmp
#define _tcscpy wcscpy
#define _tcscspn wcscspn
#define _tcslen wcslen
#define _tcsncat wcsncat
#define _tcsncmp wcsncmp
#define _tcsncpy wcsncpy
#define _tcspbrk wcspbrk
#define _tcsrchr wcsrchr
#define _tcsspn wcsspn
#define _tcsstr wcsstr
#define _tcstok wcstok
#define _tcsdup wcsdup
#define _tcsicmp _wcsicmp
#define _tcsnicmp _wcsnicmp
#define _tcsnset _wcsnset
#define _tcsrev _wcsrev
#define _tcsset _wcsset
#define _tcslwr _wcslwr
#define _tcsupr _wcsupr
#define _tcsxfrm wcsxfrm
#define _tcscoll wcscoll
#define _tcsicoll _wcsicoll
#define _istalpha iswalpha
#define _istupper iswupper
#define _istlower iswlower
#define _istdigit iswdigit
#define _istxdigit iswxdigit
#define _istspace iswspace
#define _istpunct iswpunct
#define _istalnum iswalnum
#define _istprint iswprint
#define _istgraph iswgraph
#define _istcntrl iswcntrl
#define _istascii iswascii
#define _totupper towupper
#define _totlower towlower
#define _tcsftime wcsftime
 /* Macro functions */
#define _tcsdec _wcsdec
#define _tcsinc _wcsinc
#define _tcsnbcnt _wcsncnt
#define _tcsnccnt _wcsncnt
#define _tcsnextc _wcsnextc
#define _tcsninc _wcsninc
#define _tcsspnp _wcsspnp
#define _wcsdec(_wcs1, _wcs2) ((_wcs1)>=(_wcs2) ? nullptr : (_wcs2)-1)
#define _wcsinc(_wcs) ((_wcs)+1)
#define _wcsnextc(_wcs) ((unsigned int) *(_wcs))
#define _wcsninc(_wcs, _inc) (((_wcs)+(_inc)))
#define _wcsncnt(_wcs, _cnt) ((wcslen(_wcs)>_cnt) ? _count : wcslen(_wcs))
#define _wcsspnp(_wcs1, _wcs2) ((*((_wcs1)+wcsspn(_wcs1,_wcs2))) ? ((_wcs1)+wcsspn(_wcs1,_wcs2)) : nullptr)
 
#if 1 /* defined __MSVCRT__ */
 /*
 * These wide functions not in crtdll.dll.
 * Define macros anyway so that _wfoo rather than _tfoo is undefined
 */
#define _ttoi64 _wtoi64
#define _i64tot _i64tow
#define _ui64tot _ui64tow
#define _tasctime _wasctime
#define _tctime _wctime

#if __MSVCRT_VERSION__ >= 0x0800
#define _tctime32 _wctime32
 #define _tctime64 _wctime64
 #endif /* __MSVCRT_VERSION__ >= 0x0800 */

#define _tstrdate _wstrdate
 #define _tstrtime _wstrtime
 #define _tutime _wutime
 
#if __MSVCRT_VERSION__ >= 0x0800
#define _tutime64 _wutime64
#define _tutime32 _wutime32
#endif /* __MSVCRT_VERSION__ > 0x0800 */

#define _tcsnccoll _wcsncoll
#define _tcsncoll _wcsncoll
#define _tcsncicoll _wcsnicoll
#define _tcsnicoll _wcsnicoll
#define _taccess _waccess
#define _tchmod _wchmod
#define _tcreat _wcreat
#define _tfindfirst _wfindfirst
#define _tfindnext _wfindnext

#if __MSVCRT_VERSION__ >= 0x0800
#define _tfindfirst64 _wfindfirst64
#define _tfindfirst32 _wfindfirst32
#define _tfindnext64 _wfindnext64
#define _tfindnext32 _wfindnext32
#endif /* __MSVCRT_VERSION__ > 0x0800 */

#define _tfdopen _wfdopen
#define _tfopen _wfopen
#define _tfreopen _wfreopen
#define _tfsopen _wfsopen
#define _tgetenv _wgetenv
#define _tputenv _wputenv
#define _tsearchenv _wsearchenv
#define _tsystem _wsystem
#define _tmakepath _wmakepath
#define _tsplitpath _wsplitpath
#define _tfullpath _wfullpath
#define _tmktemp _wmktemp
#define _topen _wopen
#define _tremove _wremove
#define _trename _wrename
#define _tsopen _wsopen
#define _tsetlocale _wsetlocale
#define _tunlink _wunlink
#define _tfinddata_t _wfinddata_t
#define _tfindfirsti64 _wfindfirsti64
#define _tfindnexti64 _wfindnexti64
#define _tfinddatai64_t _wfinddatai64_t

#if __MSVCRT_VERSION__ >= 0x0601
#define _tfinddata64_t _wfinddata64_t
#endif

#if __MSVCRT_VERSION__ >= 0x0800
#define _tfinddata32_t _wfinddata32_t
#define _tfinddata32i64_t _wfinddata32i64_t
#define _tfinddata64i32_t _wfinddata64i32_t
#define _tfindfirst32i64 _wfindfirst32i64
#define _tfindfirst64i32 _wfindfirst64i32
#define _tfindnext32i64 _wfindnext32i64
#define _tfindnext64i32 _wfindnext64i32
#endif /* __MSVCRT_VERSION__ > 0x0800 */

#define _tchdir _wchdir
#define _tgetcwd _wgetcwd
#define _tgetdcwd _wgetdcwd
#define _tmkdir _wmkdir
#define _trmdir _wrmdir

#ifdef __CC_WINDOWS__
#define _tstat _wstat
#define _tstati64 _wstati64
#define _tstat64 _wstat64
#else
#define _tstat wstat
#define _tstati64 wstat64
#define _tstat64 wstat64
#endif

#if __MSVCRT_VERSION__ >= 0x0800
#define _tstat32 _wstat32
#define _tstat32i64 _wstat32i64
#define _tstat64i32 _wstat64i32
#endif /* __MSVCRT_VERSION__ > 0x0800 */
#endif /* __MSVCRT__ */
 
/* dirent structures and functions */
#define _tdirent _wdirent
#define _TDIR _WDIR
#define _topendir _wopendir
#define _tclosedir _wclosedir
#define _treaddir _wreaddir
#define _trewinddir _wrewinddir
#define _ttelldir _wtelldir
#define _tseekdir _wseekdir
 
#else /* Not _UNICODE */
 
/*
* _TEOF, the constant you should use instead of EOF.
*/
#define _TEOF EOF
 
/*
* __TEXT is a private macro whose specific use is to force the expansion of a
* macro passed as an argument to the macros _T or _TEXT. DO NOT use this
* macro within your programs. It's name and function could change without
* notice.
*/
#define __TEXT(q) q
 
/* for porting from other Windows compilers */
#define _tmain main
#define _tWinMain WinMain
#define _tenviron _environ
#define __targv __argv
 
/*
* Non-unicode (standard) functions
*/
 
#define _tprintf printf
#define _ftprintf fprintf
#define _stprintf sprintf

#define _sntprintf _snprintf
#define _vsntprintf _vsnprintf

#define _vtprintf vprintf
#define _vftprintf vfprintf
#define _vstprintf vsprintf

#define _vsctprintf _vscprintf
#define _tscanf scanf
#define _ftscanf fscanf
#define _stscanf sscanf
#define _fgettc fgetc
#define _fgettchar _fgetchar
#define _fgetts fgets
#define _fputtc fputc
#define _fputtchar _fputchar
#define _fputts fputs
#define _tfdopen _fdopen
#define _tfopen fopen
#define _tfreopen freopen
#define _tfsopen _fsopen
#define _tgetenv getenv
#define _tputenv _putenv
#define _tsearchenv _searchenv
#define _tsystem system
#define _tmakepath _makepath
#define _tsplitpath _splitpath
#define _tfullpath _fullpath
#define _gettc getc
#define _getts gets
#define _puttc putc
#define _puttchar putchar
#define _putts puts
#define _ungettc ungetc
#define _tcstod strtod
#define _tcstol strtol
#define _tcstoul strtoul
#define _itot _itoa
#define _ltot _ltoa
#define _ultot _ultoa
#define _ttoi atoi
#define _ttol atol
#define _ttof atof
#define _tcscat strcat
#define _tcschr strchr
#define _tcscmp strcmp
#define _tcscpy strcpy
#define _tcscspn strcspn
#define _tcslen strlen
#define _tcsncat strncat
#define _tcsncmp strncmp
#define _tcsncpy strncpy
#define _tcspbrk strpbrk
#define _tcsrchr strrchr
#define _tcsspn strspn
#define _tcsstr strstr
#define _tcstok strtok
#define _tcsdup strdup
#define _tcsicmp _stricmp
#define _tcsnicmp _strnicmp
#define _tcsnset _strnset
#define _tcsrev _strrev
#define _tcsset _strset
#define _tcslwr _strlwr
#define _tcsupr _strupr
#define _tcsxfrm strxfrm
#define _tcscoll strcoll
#define _tcsicoll _stricoll
#define _istalpha isalpha
#define _istupper isupper
#define _istlower islower
#define _istdigit isdigit
#define _istxdigit isxdigit
#define _istspace isspace
#define _istpunct ispunct
#define _istalnum isalnum
#define _istprint isprint
#define _istgraph isgraph
#define _istcntrl iscntrl
#define _istascii isascii
#define _totupper toupper
#define _totlower tolower
#define _tasctime asctime
#define _tctime ctime

#if __MSVCRT_VERSION__ >= 0x0800
#define _tctime32 _ctime32
#define _tctime64 _ctime64
#endif /* __MSVCRT_VERSION__ >= 0x0800 */
 
#define _tstrdate _strdate
#define _tstrtime _strtime
#define _tutime _utime
 
#if __MSVCRT_VERSION__ >= 0x0800
#define _tutime64 _utime64
#define _tutime32 _utime32
#endif /* __MSVCRT_VERSION__ > 0x0800 */
 
#define _tcsftime strftime
 
/* Macro functions */
#define _tcsdec _strdec
#define _tcsinc _strinc
#define _tcsnbcnt _strncnt
#define _tcsnccnt _strncnt
#define _tcsnextc _strnextc
#define _tcsninc _strninc
#define _tcsspnp _strspnp
#define _strdec(_str1, _str2) ((_str1)>=(_str2) ? nullptr : (_str2)-1)
#define _strinc(_str) ((_str)+1)
#define _strnextc(_str) ((unsigned int) *(_str))
#define _strninc(_str, _inc) (((_str)+(_inc)))
#define _strncnt(_str, _cnt) ((strlen(_str)>_cnt) ? _count : strlen(_str))
#define _strspnp(_str1, _str2) ((*((_str1)+strspn(_str1,_str2))) ? ((_str1)+strspn(_str1,_str2)) : nullptr)
 
#define _tchmod _chmod
#define _tcreat _creat
#define _tfindfirst _findfirst
#define _tfindnext _findnext

#if __MSVCRT_VERSION__ >= 0x0800
#define _tfindfirst64 _findfirst64
#define _tfindfirst32 _findfirst32
#define _tfindnext64 _findnext64
#define _tfindnext32 _findnext32
#endif /* __MSVCRT_VERSION__ > 0x0800 */

#define _tmktemp _mktemp
#define _taccess _access
#define _tremove remove
#define _trename rename
#define _tsopen _sopen
#define _tsetlocale setlocale
#define _tunlink _unlink
#define _tfinddata_t _finddata_t
#define _tchdir _chdir
#define _tgetcwd _getcwd
#define _tgetdcwd _getdcwd
#define _tmkdir _mkdir
#define _trmdir _rmdir

#ifdef __CC_WINDOWS__
#define _topen _open
#define _tstat _stat
#define _tstati64 _stati64
#define _tstat64 _stat64
#else
#define _topen open
#define _tstat stat
#define _tstati64 stat64
#define _tstat64 stat64
#endif

#if 1 /* defined __MSVCRT__ */
/* Not in crtdll.dll. Define macros anyway? */
#define _ttoi64 _atoi64
#define _i64tot _i64toa
#define _ui64tot _ui64toa
#define _tcsnccoll _strncoll
#define _tcsncoll _strncoll
#define _tcsncicoll _strnicoll
#define _tcsnicoll _strnicoll
#define _tfindfirsti64 _findfirsti64
#define _tfindnexti64 _findnexti64
#define _tfinddatai64_t _finddatai64_t
 
#if __MSVCRT_VERSION__ >= 0x0601
#define _tfinddata64_t _finddata64_t
#endif
 
#if __MSVCRT_VERSION__ >= 0x0800
#define _tfinddata32_t _finddata32_t
#define _tfinddata32i64_t _finddata32i64_t
#define _tfinddata64i32_t _finddata64i32_t
#define _tfindfirst32i64 _findfirst32i64
#define _tfindfirst64i32 _findfirst64i32
#define _tfindnext32i64 _findnext32i64
#define _tfindnext64i32 _findnext64i32
#endif /* __MSVCRT_VERSION__ > 0x0800 */
 
#if __MSVCRT_VERSION__ >= 0x0800
#define _tstat32 _stat32
#define _tstat32i64 _stat32i64
#define _tstat64i32 _stat64i32
#endif /* __MSVCRT_VERSION__ > 0x0800 */

#endif /* __MSVCRT__ */
 
/* dirent structures and functions */
#define _tdirent dirent
#define _TDIR DIR
#define _topendir opendir
#define _tclosedir closedir
#define _treaddir readdir
#define _trewinddir rewinddir
#define _ttelldir telldir
#define _tseekdir seekdir
 
#endif /* Not _CC_UNICODE_ */
 
/*
* UNICODE a constant string when _CC_UNICODE_ is defined else returns the string
* unmodified. Also defined in w32api/winnt.h.
*/
#define _TEXT(x) __TEXT(x)
#define _T(x) __TEXT(x)
 
#endif /* Not _TCHAR_H_ */
    

#if defined(__GNUC__) && defined(_CC_UNICODE_)

#ifdef __MSVCRT__
//#error Unicode main function requires linking to MSVCRT

#include <wchar.h>
#include <stdlib.h>

#undef _tmain

void __wgetmainargs(int*,wchar_t***,wchar_t***,int,int*);

#ifdef MAIN_USE_ENVP
    int wmain(int argc, wchar_t *argv[], wchar_t *envp[]);
#else
    int wmain(int argc, wchar_t *argv[]);
#endif

extern int ___CRT_glob;
int main(void) {
    wchar_t **enpv, **argv;
    int argc = 0, si = 0;
    __wgetmainargs(&argc, &argv, &enpv, ___CRT_glob, &si); // this also creates the global variable __wargv
#ifdef MAIN_USE_ENVP
    return wmain(argc, argv, enpv);
#else
    return wmain(argc, argv);
#endif    /* #ifdef MAIN_USE_ENVP */
}
#endif /* __MSVCRT__*/
#endif /* __GNUC__*/

#undef _tmain
#ifdef _CC_UNICODE_
    #define _tmain wmain
#else
    #define _tmain main
#endif
    
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*ndef __AUTO_TCHAR_H_USESYS__ */

#endif /*_C_CC_TCHAR_H_INCLUDED_*/
