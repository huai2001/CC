#include <stdio.h>
#include <libcc/alloc.h>
#include <libcc/core.h>
#include <libcc/atomic.h>
#include <libcc/string.h>
#include <libcc/dirent.h>

struct {
    _cc_folder_t folder;
    const tchar_t *folder_title;
} folders[] = {
    {_CC_FOLDER_HOME_, _T("Home")},
    {_CC_FOLDER_DESKTOP_, _T("Desktop")},
    {_CC_FOLDER_DOCUMENTS_, _T("Document")},
    {_CC_FOLDER_DOWNLOADS_, _T("Download")},
    {_CC_FOLDER_MUSIC_, _T("Music")},
    {_CC_FOLDER_PICTURES_, _T("Pictures")},
    {_CC_FOLDER_PUBLICSHARE_, _T("Publicshare")},
    {_CC_FOLDER_SAVEDGAMES_, _T("SavedGames")},
    {_CC_FOLDER_SCREENSHOTS_, _T("Screenshots")},
    {_CC_FOLDER_TEMPLATES_, _T("Templates")},
    {_CC_FOLDER_VIDEOS_, _T("Videos")}
};

void print_dir(tchar_t* dirString) {
    tchar_t path[_CC_MAX_PATH_] = { 0 };
    DIR* dir = opendir(dirString);
    struct dirent* d = nullptr;

    while ((d = readdir(dir)) != nullptr) {
        //
        if ((d->d_name[0] == '.' && d->d_name[1] == 0) ||
            (d->d_name[0] == '.' && d->d_name[1] == '.' && d->d_name[2] == 0)) {
            continue;
        }

        _tcsncpy(path, dirString, _CC_MAX_PATH_);
        path[_CC_MAX_PATH_ - 1] = 0;
        _tcscat(path, _CC_SLASH_S_);
        _tcscat(path, d->d_name);

        if (d->d_type == DT_DIR) {
            printf("%s\n", path);
            print_dir(path);
        } else {
            printf("%s(%d)\n", d->d_name, d->d_reclen);
        }
    }
    closedir(dir);
    printf("-----------------\n");
}

int main(int argc, char* const argv[]) {
    int i;
    tchar_t path[_CC_MAX_PATH_];
    _cc_get_cwd(path, _cc_countof(path));
    _tprintf(_T("CurrentPath: %s\n"), path);

    if (argc == 2)
        print_dir(argv[1]);
    else
        print_dir(path);

    _cc_get_base_path(path,_CC_MAX_PATH_);
    _tprintf(_T("BasePath: %s\n"), path);
    _cc_get_executable_path(path,_CC_MAX_PATH_);
    _tprintf(_T("ExecutablePath: %s\n"), path);

    _cc_fpath(path,_CC_MAX_PATH_,"../examples");
    _tprintf(_T("fpath: %s\n"), path);

    for (i = 0; i < _cc_countof(folders);i++) {
        _cc_get_folder(folders[i].folder, path, _CC_MAX_PATH_);
        _tprintf(_T("%s: %s\n"), folders[i].folder_title, path);
    }


    return 0;
}
