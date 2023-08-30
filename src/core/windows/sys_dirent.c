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
#include <cc/alloc.h>
#include <cc/dirent.h>
#include <cc/string.h>

#if !defined(__CC_WIN32_CE__)
#include <fcntl.h>
#endif

#include <io.h>

#if defined(__CC_WIN32_CE__)

/*-- Called fileio.c, process.c, intrface.cpp */
int stat(const tchar_t *path, struct stat *buffer) {
    tchar_t szPath[_MAX_PATH];
    HANDLE hFind;
    WIN32_FIND_DATA w32fd;
    uint64_t dwl;
    /* stat() is called on both the ZIP files and extracted files.*/

    /* Clear our stat buffer to be safe.*/
    ZeroMemory(buffer, sizeof(struct stat));

    /* Find the file/direcotry and fill in a WIN32_FIND_DATA structure.*/
    ZeroMemory(&w32fd, sizeof(w32fd));

    _tcsncpy(szPath, path, _countof(szPath));
    szPath[_MAX_PATH - 1] = _T('\0');

    hFind = FindFirstFile(szPath, &w32fd);

    /* Bail out now if we could not find the file/directory.*/
    if (hFind == INVALID_HANDLE_VALUE) {
        return -1;
    }

    /* Close the find. */
    FindClose(hFind);

    /* Mode flags that are currently used: S_IWRITE, S_IFMT, S_IFDIR, S_IEXEC */
    if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        buffer->st_mode = _S_IFDIR | _S_IREAD | _S_IEXEC;
    } else {
        buffer->st_mode = _S_IFREG | _S_IREAD;
    }
    if (!(w32fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) {
        buffer->st_mode |= _S_IWRITE;
    }

    /* Store the file size. */
    buffer->st_size = (long)w32fd.nFileSizeLow;

    /* Convert the modified FILETIME to a time_t and store it. */
    dwl = *(uint64_t *)&w32fd.ftLastWriteTime;
    buffer->st_mtime = (time_t)((dwl - (uint64_t)116444736000000000) / (uint64_t)10000000);

    return 0;
}
#endif

/**/
DIR *opendir(const tchar_t *dir_path) {
    HANDLE handle;
    DIR *dp;

    int32_t dir_len;
    int32_t index;
    tchar_t *dirs;

    _cc_assert(dir_path != NULL);
    if (dir_path == NULL) {
        return NULL;
    }

    dir_len = (int32_t)_tcslen(dir_path);
    dirs = _CC_MALLOCX(tchar_t, (dir_len + 5));
    dp = _CC_MALLOC(DIR);
    /* copy '\0'*/
    _tcsncpy(dirs, dir_path, dir_len + 1);
    *(dirs + dir_len) = 0;

    if (dir_len > 0) {
        index = dir_len - 1;
    } else {
        index = 0;
    }

    if ((dirs[index] == _T('/') || dirs[index] == _T('\\')) && (index == 0 || !IsDBCSLeadByte((BYTE)dirs[index - 1]))) {
        dirs[index + 1] = _T('*');
        dirs[index + 2] = 0;
    } else {
        dirs[dir_len] = _T('\\');
        dirs[dir_len + 1] = _T('*');
        dirs[dir_len + 2] = 0;
    }

    dp->offset = 0;
    dp->finished = 0;

    // find dir
    handle =
        FindFirstFileEx(dirs, FindExInfoBasic, &(dp->fileinfo), FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
    if (handle == INVALID_HANDLE_VALUE) {
        int32_t err = GetLastError();
        if (err == ERROR_NO_MORE_FILES || err == ERROR_FILE_NOT_FOUND) {
            dp->finished = 1;
        } else {
            _cc_free(dp);
            _cc_free(dirs);
            return NULL;
        }
    }

    *(dirs + dir_len) = 0;

    dp->dir = dirs;
    dp->handle = handle;

    return dp;
}

/**/
struct dirent *readdir(DIR *dp) {
    int16_t n;
    DWORD attr;

    if (!dp || dp->finished) {
        return NULL;
    }

    if (dp->offset != 0) {
        if (FindNextFile(dp->handle, &(dp->fileinfo)) == 0) {
            dp->finished = 1;
            return NULL;
        }
    }
    dp->offset++;

    /*
     * Copy file name.  If the file name is too
     * long to fit in to the destination buffer, then truncate file name
     * to _MAX_FNAME characters and zero-terminate the buffer.
     */
    n = 0;
    while (((n + 1) < _MAX_FNAME) && dp->fileinfo.cFileName[n] != 0) {
        dp->dent.d_name[n] = dp->fileinfo.cFileName[n];
        n++;
    }
    dp->dent.d_name[n] = 0;

    /* File type */
    attr = dp->fileinfo.dwFileAttributes;

    if (_CC_ISSET_BIT(FILE_ATTRIBUTE_DEVICE, attr) != 0) {
        dp->dent.d_type = DT_CHR;
    } else if (_CC_ISSET_BIT(FILE_ATTRIBUTE_DIRECTORY, attr) != 0) {
        dp->dent.d_type = DT_DIR;
    } else {
        dp->dent.d_type = DT_REG;
    }

    /* Length of file name excluding zero terminator */
    dp->dent.d_namlen = n;

    dp->dent.d_ino = 1;
    dp->dent.d_reclen = sizeof(struct dirent);
    dp->dent.d_off = dp->offset;

    return &(dp->dent);
}

/**/
int readdir_r(DIR *dp, struct dirent *entry, struct dirent **result) {
    int16_t n;
    DWORD attr;

    if (!dp || dp->finished) {
        *result = NULL;
        return 0;
    }

    if (dp->offset != 0) {
        if (FindNextFile(dp->handle, &(dp->fileinfo)) == 0) {
            dp->finished = 1;
            *result = NULL;
            return 0;
        }
    }
    dp->offset++;

    /*
     * Copy file name.  If the file name is too
     * long to fit in to the destination buffer, then truncate file name
     * to _MAX_FNAME characters and zero-terminate the buffer.
     */
    n = 0;
    while (((n + 1) < _MAX_FNAME) && dp->fileinfo.cFileName[n] != 0) {
        dp->dent.d_name[n] = dp->fileinfo.cFileName[n];
        n++;
    }
    dp->dent.d_name[n] = 0;

    /* File type */
    attr = dp->fileinfo.dwFileAttributes;

    if (_CC_ISSET_BIT(FILE_ATTRIBUTE_DEVICE, attr) != 0) {
        dp->dent.d_type = DT_CHR;
    } else if (_CC_ISSET_BIT(FILE_ATTRIBUTE_DIRECTORY, attr) != 0) {
        dp->dent.d_type = DT_DIR;
    } else {
        dp->dent.d_type = DT_REG;
    }

    /* Length of file name excluding zero terminator */
    dp->dent.d_namlen = n;

    dp->dent.d_ino = 1;
    dp->dent.d_reclen = sizeof(struct dirent);
    dp->dent.d_off = dp->offset;

    memcpy(entry, &dp->dent, sizeof(struct dirent));

    *result = &dp->dent;

    return 0;
}

/**/
int closedir(DIR *dp) {
    if (!dp) {
        return 0;
    }

    /* It is valid to scan an empty directory but we have an invalid
     handle in this case (no first file found). */
    if (dp->handle != INVALID_HANDLE_VALUE) {
        FindClose(dp->handle);
    }

    _cc_free(dp->dir);
    _cc_free(dp);

    return 0;
}

/**/
bool_t _cc_isdir(const tchar_t *dir_path) {
    DWORD dw;
    dw = GetFileAttributes(dir_path);

    if (dw != (DWORD)-1 && dw == FILE_ATTRIBUTE_DIRECTORY) {
        return true;
    }

    return false;
}