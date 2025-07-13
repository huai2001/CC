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
#include <libcc/alloc.h>
#include <libcc/dirent.h>
#include <libcc/string.h>
#include <libcc/math.h>
#include <shlobj.h>
#include <initguid.h>

#if !defined(__CC_WIN32_CE__)
#include <fcntl.h>
#endif

#include <io.h>

#if defined(__CC_WIN32_CE__)

/*-- Called fileio.c, process.c, intrface.cpp */
_CC_API_PUBLIC(int) stat(const tchar_t *path, struct stat *buffer) {
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
_CC_API_PUBLIC(DIR*) opendir(const tchar_t *dir_path) {
    HANDLE handle;
    DIR *dp;

    size_t dir_len;
    size_t index;
    tchar_t *dirs;

    _cc_assert(dir_path != nullptr);
    if (dir_path == nullptr) {
        return nullptr;
    }

    dp = _CC_MALLOC(DIR);
    dir_len = _tcslen(dir_path);
    dirs = _cc_tcsndup(dir_path, dir_len);
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
    handle = FindFirstFileEx(dirs, FindExInfoBasic, &(dp->fileinfo), FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
    if (handle == INVALID_HANDLE_VALUE) {
        int32_t err = GetLastError();
        if (err == ERROR_NO_MORE_FILES || err == ERROR_FILE_NOT_FOUND) {
            dp->finished = 1;
        } else {
            _cc_free(dp);
            _cc_free(dirs);
            return nullptr;
        }
    }

    dirs[dir_len] = 0;

    dp->dir = dirs;
    dp->handle = handle;

    return dp;
}

/**/
_CC_API_PUBLIC(struct dirent*) readdir(DIR *dp) {
    DWORD attr;
    uint16_t n = 0;
    if (!dp || dp->finished) {
        return nullptr;
    }

    if (dp->offset != 0) {
        if (FindNextFile(dp->handle, &(dp->fileinfo)) == 0) {
            dp->finished = 1;
            return nullptr;
        }
    }
    dp->offset++;

    /*
     * Copy file name.  If the file name is too
     * long to fit in to the destination buffer, then truncate file name
     * to _MAX_FNAME characters and zero-terminate the buffer.
     */

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

    dp->dent.d_ino = 1;
    dp->dent.d_reclen = n;
    dp->dent.d_off = dp->offset;

    return &(dp->dent);
}

/**/
_CC_API_PUBLIC(int) readdir_r(DIR *dp, struct dirent *entry, struct dirent **result) {
    int16_t n;
    DWORD attr;

    if (!dp || dp->finished) {
        *result = nullptr;
        return 1;
    }

    if (dp->offset != 0) {
        if (FindNextFile(dp->handle, &(dp->fileinfo)) == 0) {
            dp->finished = 1;
            *result = nullptr;
            return 1;
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

    dp->dent.d_ino = 1;
    dp->dent.d_reclen = n;
    dp->dent.d_off = dp->offset;

    memcpy(entry, &dp->dent, sizeof(struct dirent));

    *result = &dp->dent;

    return 0;
}

/**/
_CC_API_PUBLIC(int) closedir(DIR *dp) {
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
_CC_API_PUBLIC(bool_t) _cc_isdir(const tchar_t *dir_path) {
    DWORD dw;
    dw = GetFileAttributes(dir_path);

    if (dw != (DWORD)-1 && dw == FILE_ATTRIBUTE_DIRECTORY) {
        return true;
    }

    return false;
}

// These aren't all defined in older SDKs, so define them here
DEFINE_GUID(_CC_FOLDERID_PROFILE_, 0x5E6C858F, 0x0E22, 0x4760, 0x9A, 0xFE, 0xEA, 0x33, 0x17, 0xB6, 0x71, 0x73);
DEFINE_GUID(_CC_FOLDERID_DESKTOP_, 0xB4BFCC3A, 0xDB2C, 0x424C, 0xB0, 0x29, 0x7F, 0xE9, 0x9A, 0x87, 0xC6, 0x41);
DEFINE_GUID(_CC_FOLDERID_DOCUMENTS_, 0xFDD39AD0, 0x238F, 0x46AF, 0xAD, 0xB4, 0x6C, 0x85, 0x48, 0x03, 0x69, 0xC7);
DEFINE_GUID(_CC_FOLDERID_DOWNLOADS_, 0x374de290, 0x123f, 0x4565, 0x91, 0x64, 0x39, 0xc4, 0x92, 0x5e, 0x46, 0x7b);
DEFINE_GUID(_CC_FOLDERID_MUSIC_, 0x4BD8D571, 0x6D19, 0x48D3, 0xBE, 0x97, 0x42, 0x22, 0x20, 0x08, 0x0E, 0x43);
DEFINE_GUID(_CC_FOLDERID_PICTURES_, 0x33E28130, 0x4E1E, 0x4676, 0x83, 0x5A, 0x98, 0x39, 0x5C, 0x3B, 0xC3, 0xBB);
DEFINE_GUID(_CC_FOLDERID_SAVEDGAMES_, 0x4c5c32ff, 0xbb9d, 0x43b0, 0xb5, 0xb4, 0x2d, 0x72, 0xe5, 0x4e, 0xaa, 0xa4);
DEFINE_GUID(_CC_FOLDERID_SCREENSHOTS_, 0xb7bede81, 0xdf94, 0x4682, 0xa7, 0xd8, 0x57, 0xa5, 0x26, 0x20, 0xb8, 0x6f);
DEFINE_GUID(_CC_FOLDERID_TEMPLATES_, 0xA63293E8, 0x664E, 0x48DB, 0xA0, 0x79, 0xDF, 0x75, 0x9E, 0x05, 0x09, 0xF7);
DEFINE_GUID(_CC_FOLDERID_VIDEOS_, 0x18989B1D, 0x99B5, 0x455B, 0x84, 0x1C, 0xAB, 0x7C, 0x74, 0xE4, 0xDD, 0xFC);

_CC_API_PUBLIC(size_t) _cc_get_executable_path(tchar_t *path, size_t len) {
    len = GetModuleFileName(nullptr, path, (DWORD)len);
    if (len == 0) {
        _cc_logger_error(_T("Couldn't locate our .exe"));
        return 0;
    }
    return len;
}

_CC_API_PUBLIC(size_t) _cc_get_base_path(tchar_t *path, size_t len) {
    size_t i;
    len = GetModuleFileName(nullptr, path, (DWORD)len);
    if (len == 0) {
        _cc_logger_error(_T("Couldn't locate our .exe"));
        return 0;
    }

    for (i = len - 1; i > 0; i--) {
        if (path[i] == '\\') {
            break;
        }
    }

    _cc_assert(i > 0);  // Should have been an absolute path.
    path[i + 1] = '\0'; // chop off filename.

    return i;
}


_CC_API_PUBLIC(size_t) _cc_get_folder(_cc_folder_t folder, tchar_t *path, size_t len) {
    typedef HRESULT (WINAPI *pfnSHGetKnownFolderPath)(REFGUID /* REFKNOWNFOLDERID */, DWORD, HANDLE, PWSTR*);
    HMODULE lib = LoadLibraryW(L"Shell32.dll");
    pfnSHGetKnownFolderPath pSHGetKnownFolderPath = nullptr;
    size_t length = 0;

    if (lib) {
        pSHGetKnownFolderPath = (pfnSHGetKnownFolderPath)GetProcAddress(lib, "SHGetKnownFolderPath");
    }

    if (pSHGetKnownFolderPath) {
        GUID type;
        HRESULT hr;
        PWSTR pszPath = nullptr;

        switch (folder) {
        case _CC_FOLDER_HOME_:
            type = _CC_FOLDERID_PROFILE_;
            break;

        case _CC_FOLDER_DESKTOP_:
            type = _CC_FOLDERID_DESKTOP_;
            break;

        case _CC_FOLDER_DOCUMENTS_:
            type = _CC_FOLDERID_DOCUMENTS_;
            break;

        case _CC_FOLDER_DOWNLOADS_:
            type = _CC_FOLDERID_DOWNLOADS_;
            break;

        case _CC_FOLDER_MUSIC_:
            type = _CC_FOLDERID_MUSIC_;
            break;

        case _CC_FOLDER_PICTURES_:
            type = _CC_FOLDERID_PICTURES_;
            break;

        case _CC_FOLDER_PUBLICSHARE_:
            _cc_logger_error(_T("Public share unavailable on Windows"));
            goto done;

        case _CC_FOLDER_SAVEDGAMES_:
            type = _CC_FOLDERID_SAVEDGAMES_;
            break;

        case _CC_FOLDER_SCREENSHOTS_:
            type = _CC_FOLDERID_SCREENSHOTS_;
            break;

        case _CC_FOLDER_TEMPLATES_:
            type = _CC_FOLDERID_TEMPLATES_;
            break;

        case _CC_FOLDER_VIDEOS_:
            type = _CC_FOLDERID_VIDEOS_;
            break;

        default:
            _cc_logger_error(_T("Invalid _cc_folder_t: %d"), (int)folder);
            goto done;
        };

        hr = pSHGetKnownFolderPath(&type, 0x00008000 /* KF_FLAG_CREATE */, nullptr, &pszPath);
        if (SUCCEEDED(hr)) {
            length = wcslen(pszPath);
#ifdef _CC_UNICODE_
            length = _min(length,len);
            memcpy(path, pszPath, length);
            path[length - 1] = 0;
#else
            length = _cc_w2a(pszPath, (int32_t)length, path, (int32_t)len);
#endif
            //CoTaskMemFree(pszPath);
        } else {
            length = 0;
            _cc_logger_error(_T("Couldn't get folder, %s"), _cc_last_error(hr));
        }
    } else {
        int type;
        HRESULT hr;

        switch (folder) {
        case _CC_FOLDER_HOME_:
            type = CSIDL_PROFILE;
            break;

        case _CC_FOLDER_DESKTOP_:
            type = CSIDL_DESKTOP;
            break;

        case _CC_FOLDER_DOCUMENTS_:
            type = CSIDL_MYDOCUMENTS;
            break;

        case _CC_FOLDER_DOWNLOADS_:
            _cc_logger_error(_T("Downloads folder unavailable before Vista"));
            goto done;

        case _CC_FOLDER_MUSIC_:
            type = CSIDL_MYMUSIC;
            break;

        case _CC_FOLDER_PICTURES_:
            type = CSIDL_MYPICTURES;
            break;

        case _CC_FOLDER_PUBLICSHARE_:
            _cc_logger_error(_T("Public share unavailable on Windows"));
            goto done;

        case _CC_FOLDER_SAVEDGAMES_:
            _cc_logger_error(_T("Saved games unavailable before Vista"));
            goto done;

        case _CC_FOLDER_SCREENSHOTS_:
            _cc_logger_error(_T("Screenshots folder unavailable before Vista"));
            goto done;

        case _CC_FOLDER_TEMPLATES_:
            type = CSIDL_TEMPLATES;
            break;

        case _CC_FOLDER_VIDEOS_:
            type = CSIDL_MYVIDEO;
            break;

        default:
            _cc_logger_error(_T("Unsupported _CC_Folder_ on Windows before Vista: %d"), (int)folder);
            goto done;
        };

        // Create the OS-specific folder if it doesn't already exist
        type |= CSIDL_FLAG_CREATE;

#if 0
        // Apparently the oldest, but not supported in modern Windows
        HRESULT hr = SHGetSpecialFolderPath(nullptr, path, type, TRUE);
#endif

        /* Windows 2000/XP and later, deprecated as of Windows 10 (still
           available), available in Wine (tested 6.0.3) */
        hr = SHGetFolderPath(nullptr, type, nullptr, SHGFP_TYPE_CURRENT, path);

        // use `== TRUE` for SHGetSpecialFolderPath
        if (SUCCEEDED(hr)) {
            length = _tcslen(path);
        } else {
            length = 0;
            _cc_logger_error(_T("Couldn't get folder, %s"), _cc_last_error(hr));
        }
    }

done:
    if (lib) {
        FreeLibrary(lib);
    }
    return length;
}
#if 0
/**/
_CC_API_PUBLIC(bool_t) _cc_create_director(const tchar_t *path) {
    DWORD rc = CreateDirectory(path, nullptr);
    if (!rc && (GetLastError() == ERROR_ALREADY_EXISTS)) {
        WIN32_FILE_ATTRIBUTE_DATA winstat;
        if (GetFileAttributesExW(path, GetFileExInfoStandard, &winstat)) {
            if (winstat.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                rc = 1;  // exists and is already a directory: cool.
            }
        }
    }
    if (!rc) {
        return false;
    }
    return true;
}
#endif
/**/
_CC_API_PUBLIC(size_t) _cc_get_cwd(tchar_t *path, size_t length) {
    while (true) {
        const DWORD bw = GetCurrentDirectory((DWORD)length, path);
        if (bw == 0) {
            return 0;
        } else if (bw < length) {  // we got it!
            // make sure there's a path separator at the end.
            if ((bw == 0) || (path[bw - 1] != '\\')) {
                path[bw] = '\\';
                path[bw + 1] = '\0';
            }
            return bw;
        }
    }
    return 0;
}

/*
_CC_API_PUBLIC(bool_t) _sys_get_path_info(const tchar_t *path, _cc_path_info_t *info) {
    WIN32_FILE_ATTRIBUTE_DATA winstat;
    const BOOL rc = GetFileAttributesEx(path, GetFileExInfoStandard, &winstat);
    if (!rc) {
        return false;
    }

    if (winstat.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        info->type = _CC_PATHTYPE_DIRECTORY_;
        info->size = 0;
    } else if (winstat.dwFileAttributes & (FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_DEVICE)) {
        info->type = _CC_PATHTYPE_OTHER_;
        info->size = ((((Uint64) winstat.nFileSizeHigh) << 32) | winstat.nFileSizeLow);
    } else {
        info->type = _CC_PATHTYPE_FILE_;
        info->size = ((((Uint64) winstat.nFileSizeHigh) << 32) | winstat.nFileSizeLow);
    }

    info->create_time = _CC_TimeFromWindows_(winstat.ftCreationTime.dwLowDateTime, winstat.ftCreationTime.dwHighDateTime);
    info->modify_time = _CC_TimeFromWindows_(winstat.ftLastWriteTime.dwLowDateTime, winstat.ftLastWriteTime.dwHighDateTime);
    info->access_time = _CC_TimeFromWindows_(winstat.ftLastAccessTime.dwLowDateTime, winstat.ftLastAccessTime.dwHighDateTime);

    return true;
}*/