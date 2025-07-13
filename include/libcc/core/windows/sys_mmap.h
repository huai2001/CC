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
#ifndef _C_CC_WINDOWS_MMAP_H_INCLUDED_
#define _C_CC_WINDOWS_MMAP_H_INCLUDED_

#include "../windows.h"

/* Protections */
#define PROT_NONE           0x00            /* no permissions */
#define PROT_READ           0x01            /* pages can be read */
#define PROT_WRITE          0x02            /* pages can be written */
#define PROT_EXEC           0x04            /* pages can be executed */

/* Sharing type and options */
#define MAP_SHARED          0x0001          /* share changes */
#define MAP_PRIVATE         0x0002          /* changes are private */
#define MAP_COPY            MAP_PRIVATE     /* Obsolete */
#define MAP_FIXED           0x0010          /* map addr must be exactly as requested */
#define MAP_RENAME          0x0020          /* Sun: rename private pages to file */
#define MAP_NORESERVE       0x0040          /* Sun: don't reserve needed swap area */
#define MAP_INHERIT         0x0080          /* region is retained after exec */
#define MAP_NOEXTEND        0x0100          /* for MAP_FILE, don't change file size */
#define MAP_HASSEMAPHORE    0x0200          /* region may contain semaphores */
#define MAP_STACK           0x0400          /* region grows down, like a stack */

/* Error returned from mmap() */
#define MAP_FAILED          ((void *)-1)

/* Flags to msync */
#define MS_ASYNC            0x01            /* perform asynchronous writes */
#define MS_SYNC             0x02            /* perform synchronous writes */
#define MS_INVALIDATE       0x04            /* invalidate cached data */

/* File modes for 'open' not defined in MinGW32  (not used by mmap) */
#ifndef S_IWGRP
#define S_IWGRP             0
#define S_IRGRP             0
#define S_IROTH             0
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Map a file to a memory region
 */
_CC_API_PUBLIC(pvoid_t) mmap(pvoid_t addr, unsigned int len, int prot, int flags, int fd, unsigned int offset);
/**
 * Unmap a memory region
 */
_CC_API_PUBLIC(int) munmap(pvoid_t addr, int len);

/**
 * Synchronize a mapped region
 */
_CC_API_PUBLIC(int) msync(char *addr, int len, int flags);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_WINDOWS_FLOCK_H_INCLUDED_ */
