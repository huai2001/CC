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
#ifndef _C_CC_DIRENT_H_INCLUDED_
#define _C_CC_DIRENT_H_INCLUDED_

#include "core.h"

#if defined(__CC_WINDOWS__)
    #include "./core/windows.h"
    #ifndef __CC_WIN32_CE__
        #include <direct.h>
        #include <sys/stat.h>
    #endif
#elif defined(_CC_GCC_) || defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    #include <dirent.h>
    #include <sys/stat.h>
    #define _mkdir(file) mkdir(file, 0777)
    #define _rmdir rmdir
    #define _stat stat
    #define _unlink unlink
#else
    #include <unistd.h>
    #define _mkdir(file) mkdir(file, 0777)
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if defined(__CC_WINDOWS__)
#ifdef __CC_WIN32_CE__
    #ifndef _MAX_FNAME
        #define _MAX_FNAME MAX_PATH
    #endif

    #define _S_IFMT         0xF000          /* file type mask */
    #define _S_IFDIR        0x4000          /* directory */
    #define _S_IFCHR        0x2000          /* character special */
    #define _S_IFIFO        0x1000          /* pipe */
    #define _S_IFREG        0x8000          /* regular */
    #define _S_IREAD        0x0100          /* read permission, owner */
    #define _S_IWRITE       0x0080          /* write permission, owner */
    #define _S_IEXEC        0x0040          /* execute/search permission, owner */

    #define S_IFMT   _S_IFMT
    #define S_IFDIR  _S_IFDIR
    #define S_IFCHR  _S_IFCHR
    #define S_IFREG  _S_IFREG
    #define S_IREAD  _S_IREAD
    #define S_IWRITE _S_IWRITE
    #define S_IEXEC  _S_IEXEC

    struct stat {
        long     st_dev;
        unsigned short st_ino;
        unsigned short st_mode;
        short  st_nlink;
        short  st_uid;
        short  st_gid;
        long   st_rdev;
        long   st_size;
        time_t st_atime;
        time_t st_mtime;
        time_t st_ctime;
    };
    /**/
    _CC_API_PUBLIC(int) stat(const tchar_t *, struct stat *);
#endif /* __CC_WIN32_CE__ */
    
#ifdef __CC_CYGWIN__
    /* on Cygwin, mkdir is to be used as on POSIX OS */
    #define _mkdir mkdir(file, 0777)
#endif

/*
 * File types
 */
#define DT_UNKNOWN       0
#define DT_FIFO          1
#define DT_CHR           2
#define DT_DIR           4
#define DT_BLK           6
#define DT_REG           8
#define DT_LNK          10
#define DT_SOCK         12
#define DT_WHT          14

/*
* [XSI] The following macros shall be provided to test whether a file is
* of the specified type.  The value m supplied to the macros is the value
* of st_mode from a stat structure.  The macro shall evaluate to a non-zero
* value if the test is true; 0 if the test is false.
*/
#ifndef	S_ISDIR
#define S_ISBLK(m)       (((m) & S_IFMT) == S_IFBLK)     /* block special */
#define S_ISCHR(m)       (((m) & S_IFMT) == _S_IFCHR)    /* char special */
#define S_ISDIR(m)       (((m) & S_IFMT) == _S_IFDIR)    /* directory */
#define S_ISFIFO(m)      (((m) & S_IFMT) == _S_IFIFO)    /* fifo or socket */
#define S_ISREG(m)       (((m) & S_IFMT) == _S_IFREG)    /* regular file */
#define S_ISLNK(m)       /* symbolic link */
#define S_ISSOCK(m)      /* socket */
#endif
/* struct dirent - same as Unix */
struct dirent {
    long d_ino;                     /* inode (always 1 in WIN32) */
    off_t d_off;                    /* offset to this dirent */
    uint16_t d_reclen;              /* length of d_name */
    uint8_t d_type;                 /* file type, see below */
    tchar_t d_name[_MAX_FNAME + 1]; /* filename (nullptr terminated) */
};


/* typedef DIR - not the same as Unix */
typedef struct _dir {
    HANDLE handle;                  /* _findfirst/_findnext handle */
    int16_t offset;                 /* offset into directory */
    int16_t finished;               /* 1 if there are not more files */
    WIN32_FIND_DATA fileinfo;       /* from _findfirst/_findnext */
    tchar_t *dir;                   /* the dir we are reading */
    struct dirent dent;             /* the dirent to return */
} DIR;

/**
 * @brief read dir
 *
 * @param dp DIR context
 * 
 * @return struct dirent context
 */
_CC_API_PUBLIC(struct dirent *) readdir(DIR *dp);

/**
 * @brief read reverse dir
 *
 * @param dp DIR context
 * @param entry struct dirent context
 * @param result struct dirent context
 * 
 * @return 0 if successful or system on error.
 */
_CC_API_PUBLIC(int) readdir_r(DIR *dp, struct dirent *entry, struct dirent **result);
/**
 * @brief open dir
 *
 * @param dir_path dir path
 * 
 * @return DIR context
 */
_CC_API_PUBLIC(DIR*) opendir(const tchar_t *dir_path);
/**
 * @brief close dir
 *
 * @param 1 DIR context
 * 
 * @return 0 if successful or system on error.
 */
_CC_API_PUBLIC(int) closedir(DIR *);

#endif /* __CC_WINDOWS__ */

#define _CC_ACCESS_F_       0x00
#define _CC_ACCESS_W_       0x02
#define _CC_ACCESS_R_       0x04
#define _CC_ACCESS_X_       0x06
/**
 * @brief is dir
 *
 * @param dir_path dir path
 * 
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_isdir(const tchar_t* dir_path);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_DIRENT_H_INCLUDED_*/
