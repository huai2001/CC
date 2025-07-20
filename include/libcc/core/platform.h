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
#ifndef _C_CC_CONFIG_PLATFORM_H_INCLUDED_
#define _C_CC_CONFIG_PLATFORM_H_INCLUDED_

/**/
#if defined(_AIX) || defined(__AIX) || defined(__AIX__) || defined(__aix) || defined(__aix__)
#define __CC_AIX__     1
#define _CC_PLATFORM_NAME_ "AIX"
#endif

#if defined(__HAIKU__)
#define __CC_HAIKU__   1
#define _CC_PLATFORM_NAME_ "Haiku"
#endif

#if defined(bsdi) || defined(__bsdi) || defined(__bsdi__)
#define __CC_BSDI__    1
#define _CC_PLATFORM_NAME_ "BSDI"
#endif

#if defined(_arch_dreamcast)
#define __DREAMCAST__   1
#endif

#if defined(hpux) || defined(__hpux) || defined(__hpux__)
#define __CC_HPUX__    1
#define _CC_PLATFORM_NAME_ "HP-UX"
#endif

#if defined(sgi) || defined(__sgi) || defined(__sgi__) || defined(_SGI_SOURCE)
#define __CC_IRIX__    1
#define _CC_PLATFORM_NAME_ "SGI"
#endif

#if defined(osf) || defined(__osf) || defined(__osf__) || defined(_OSF_SOURCE)
#define __CC_OSF__     1
#define _CC_PLATFORM_NAME_ "OSF1"
#endif

#if defined(__QNX__) || defined(__QNXNTO__)
#define __CC_QNXNTO__  1
#define _CC_PLATFORM_NAME_ "QNX"
#endif


#if defined(__sun) && defined(__SVR4)
    #define __CC_SOLARIS__ 1
    #define _CC_PLATFORM_NAME_ "SunOS"
#endif

#ifdef __MINGW32__
    #define __CC_MINGW__ __MINGW32__
#elif defined(__MINGW64__)
    #define __CC_MINGW__ __MINGW64__
#endif/* defined(__MINGW32__) || defined(__MINGW64__) */

/* Define operating platform*/
#if defined(WIN32) || defined(_WIN32) || \
	defined(WIN64) || defined(_WIN64) || \
	defined(_WIN32_WCE) || defined(__CC_MINGW__)

#define __CC_WINDOWS__ 1

/* Try to find out if we're compiling for WinRT or non-WinRT */
#if defined(_CC_MSVC_) && defined(__has_include)
    #if __has_include(<winapifamily.h>)
        #define WINAPI_FAMILY_H 1
    #else
        #define WINAPI_FAMILY_H 0
    #endif
/* If _USING_V110_SDK71_ is defined it means we are using the Windows XP toolset. */
#elif defined(_CC_MSVC_) && (_CC_MSVC_ >= 1700 && !_USING_V110_SDK71_)    /* _CC_MSVC_ == 1700 for Visual Studio 2012 */
    #define WINAPI_FAMILY_H 1
#else
    #define WINAPI_FAMILY_H 0
#endif

#if WINAPI_FAMILY_H
    #include <winapifamily.h>
    #define WINAPI_FAMILY_WINRT (!WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP))
#else
    #define WINAPI_FAMILY_WINRT 0
#endif /* WINAPI_FAMILY_H */

#if WINAPI_FAMILY_WINRT
    #define __CC_WINRT__ 1
    #define _CC_PLATFORM_NAME_ "Windows RT"
#elif defined(WIN64) || defined(_WIN64)
	#define __CC_WIN64__    1
	#define _CC_PLATFORM_NAME_ "Windows 64-bit"
#elif defined(_WIN32_WCE)
	#define __CC_WIN32_CE__ 1
	#define _CC_PLATFORM_NAME_ "Windows CE"
#else
    #define __CC_WIN32__    1
    #define _CC_PLATFORM_NAME_ "Windows 32-bit"
#endif

#endif /* defined(WINDOWS) */

/* The NACL compiler defines __native_client__ and __pnacl__
 * Ref: http://www.chromium.org/nativeclient/pnacl/stability-of-the-pnacl-bitcode-abi
 */
#if defined(__native_client__)
    #undef __LINUX__
    #define __CC_NACL__ 1
    #define _CC_PLATFORM_NAME_ "__native_client__"
#endif

#if defined(__pnacl__)
    #undef __LINUX__
    #define __CC_PNACL__ 1
    #define _CC_PLATFORM_NAME_ "__pnacl__"
#endif

#if defined(__OS2__)
    #define __CC_OS2__    1
    #define _CC_PLATFORM_NAME_ "OS2"
#endif

#if defined(__CYGWIN__)
    #define __CC_CYGWIN__ 1
    #define _CC_PLATFORM_NAME_ "Cygwin"
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD) || \
    defined(__FreeBSD_kernel__) || defined(__DragonFly__)
    #define __CC_FREEBSD__ 1
    #define __CC_BSD__ __CC_FREEBSD__
    #define _CC_PLATFORM_NAME_ "FreeBSD"
#endif

#if defined(__NetBSD__) || defined(__NetBSD)
    #define __CC_NETBSD__ 1
    #define __CC_BSD__ __CC_NETBSD__
    #define _CC_PLATFORM_NAME_ "NetBSD"
#endif

#if defined(__OpenBSD__) || defined(__OpenBSD)
    #define __CC_OPENBSD__ 1
    #define __CC_BSD__ __CC_OPENBSD__
    #define _CC_PLATFORM_NAME_ "OpenBSD"
#endif

#if defined(linux) || defined(__linux) || \
    defined(__linux__) || defined(__GLIBC__)

    #define __CC_LINUX__ 1
    #define _CC_PLATFORM_NAME_ "linux"

    #define _CC_HAVE_CLOCK_GETTIME_ 1
#endif

#if defined(riscos) || defined(__riscos) || \
    defined(__riscos__)
    #define __CC_RISCOS__ 1
    #define _CC_PLATFORM_NAME_ "RISCos"
#endif

#if defined(ANDROID) || defined(__ANDROID__)
    //#undef __CC_LINUX__ /*do we need to do this?*/
    #undef _CC_PLATFORM_NAME_
    
    #define __CC_ANDROID__ 1
    #define _CC_PLATFORM_NAME_ "Google Android"
    
    #define _CC_THREAD_PTHREAD_RECURSIVE_MUTEX_ 1
    #define _CC_HAVE_CLOCK_GETTIME_ 1
#endif

#if defined(__APPLE__)
    #define __CC_APPLE__ 1
    /* lets us know what version of Mac OS X we're compiling on */
    #include <AvailabilityMacros.h>
    #include <TargetConditionals.h>
    /* if compiling for iPhone */
    #if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
        #define __CC_IPHONEOS__ 1
        #define _CC_PLATFORM_NAME_ "iOS"
    #elif defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR
        #define __CC_IPHONEOS__ 1
        #define __CC_IPHONEOS_SIMULATOR__ 1
        #define _CC_PLATFORM_NAME_ "iOS Simulator"
    #elif defined(TARGET_OS_TV) && TARGET_OS_TV
        #define __CC_APPLE_TVOS__ 1
        #define _CC_PLATFORM_NAME_ "TV OS"
    #else
        /* if not compiling for iPhone */
        #define __CC_MACOSX__ 1
        #define _CC_PLATFORM_NAME_ "macOS" 
    #endif

    #define _CC_THREAD_PTHREAD_RECURSIVE_MUTEX_ 1

#endif /* defined(__APPLE__) */

#endif /* _C_CC_CONFIG_PLATFORM_H_INCLUDED_*/
