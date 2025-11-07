#include <libcc/os.h>
#include <libcc/math.h>
#include <libcc/alloc.h>

_CC_API_PUBLIC(size_t) _cc_get_executable_path(tchar_t *path, size_t length) {
    _cc_logger_error(_T("That operation is not supported"));
    return 0;
}

_CC_API_PUBLIC(size_t) _cc_get_base_path(tchar_t *path, size_t length) {
    _cc_sds_t fpath = Android_JNI_GetExternalStoragePath();
    size_t fpath_length = _cc_sds_length(fpath);
    length = _min(fpath_length, length);
    memcpy(path, fpath, length);
    path[length - 1] = 0;
    return length;
}

_CC_API_PUBLIC(size_t) _cc_get_folder(_cc_folder_t folder, tchar_t *path, size_t length) {
    _cc_sds_t fpath;
    size_t fpath_length;
    if (folder == _CC_FOLDER_TEMPLATES_) {
        fpath = Android_JNI_GetCachePath();
    } else {
        fpath = Android_JNI_GetExternalStoragePath();
    }

    fpath_length = _cc_sds_length(fpath);
    length = _min(fpath_length, length);
    memcpy(path, fpath, length);
    path[length - 1] = 0;
    return length;
}