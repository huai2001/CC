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
#ifndef _C_CC_FILE_HEAD_FILE_
#define _C_CC_FILE_HEAD_FILE_

#include "../types.h"
#include <string.h>
#include <stdio.h>

#ifdef __CC_WINDOWS__
#include "./windows/sys_flock.h"
#endif


/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_FILE_SEEK_SET_ 0       /**< Seek from the beginning of data */
#define _CC_FILE_SEEK_CUR_ 1       /**< Seek relative to current read point */
#define _CC_FILE_SEEK_END_ 2       /**< Seek relative to the end of data */

#ifdef off64_t
    typedef off64_t _cc_fseek_off_t;
    #define _cc_fseek_off fseeko64
    #define _cc_ftell_off ftello64
#elif defined(__CC_WINDOWS__)
    typedef __int64 _cc_fseek_off_t;
    #define _cc_fseek_off _fseeki64
    #define _cc_ftell_off _ftelli64
#elif defined(__CC_APPLE__)
    typedef off_t _cc_fseek_off_t;
    #define _cc_fseek_off fseeko
    #define _cc_ftell_off ftello
#else
    #ifdef off_t
        typedef off_t _cc_fseek_off_t;
    #else
        typedef long _cc_fseek_off_t;
    #endif
    #define _cc_fseek_off fseek
    #define _cc_ftell_off ftell
#endif

typedef struct _cc_file _cc_file_t;

/**
 * @brief This is the read/write operation structure -- very basic.
 */
struct _cc_file {
    /**
     *  @return Return the size of the file in this rwops, or -1 if unknown
     */
    int64_t (*size) (_cc_file_t *context);
    /**
     *  @brief If the stream is buffering, make sure the data is written out.
     * 
     *  @return true if successful or false on write error when flushing data.
     */
    bool_t (*flush) (_cc_file_t *context);
    /**
     *  @brief Seek to \c offset relative to \c whence, one of stdio's whence values:
     *         _CC_FILE_SEEK_SET, _CC_FILE_SEEK_CUR, _CC_FILE_SEEK_END
     *
     *  @return true if successful or false on error.
     */
    int64_t (*seek) (_cc_file_t *context, int64_t offset, int whence);
    
    /**
     *  @brief Read up to \c maxnum objects each of size \c size from the data
     *         stream to the area pointed at by \c ptr.
     *
     *  @return the number of objects read, or 0 at error or end of file.
     */
    size_t (*read) (_cc_file_t *context, pvoid_t ptr, size_t size, size_t maxnum);
    
    /**
     *  @brief Write exactly \c num objects each of size \c size from the area
     *         pointed at by \c ptr to data stream.
     *
     *  @return the number of objects written, or 0 at error or end of file.
     */
    size_t (*write) (_cc_file_t *context, const pvoid_t ptr, size_t size, size_t num);
    
    /**
     *  @brief Close and free an allocated _cc_file_t structure.
     *
     *  @return true if successful or false on error.
     */
    bool_t (*close) (_cc_file_t *context);

#ifdef __CC_WINDOWS__
    bool_t append;
#endif
    pvoid_t fp;
};
/* @{ */
/**
 * @brief name Read/write macros
 *        Macros to easily read and write from an _cc_file structure.
 */
#define _cc_file_size(ctx) (ctx)->size(ctx)
#define _cc_file_seek(ctx, offset, whence) (ctx)->seek(ctx, offset, whence)
#define _cc_file_read(ctx, ptr, size, n) (ctx)->read(ctx, ptr, size, n)
#define _cc_file_write(ctx, ptr, size, n) (ctx)->write(ctx, ptr, size, n)
#define _cc_file_close(ctx) (ctx)->close(ctx)

#define _cc_fopen   _cc_open_file
#define _cc_fseek   _cc_file_seek
#define _cc_fwrite  _cc_file_write
#define _cc_fread   _cc_file_read
#define _cc_fclose  _cc_file_close
/* @} *//* Read/write macros */

/**
 * @brief Open file
 *
 * @param filename file path
 * @param mode Open file mode
 *
 * @return _cc_file structure
*/
_CC_API_PUBLIC(_cc_file_t*) _cc_open_file(const tchar_t *filename, const tchar_t *mode);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_FILE_HEAD_FILE_*/




