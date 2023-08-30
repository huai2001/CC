#include <stdio.h>
#include <cc/alloc.h>
#include <cc/core.h>
#include <cc/atomic.h>
#include <cc/string.h>
#include <cc/dirent.h>

void print_dir(tchar_t* dirString) {
    tchar_t path[_CC_MAX_PATH_] = { 0 };
    DIR* dir = opendir(dirString);
    struct dirent* d = NULL;

    while ((d = readdir(dir)) != NULL) {
        //
        if ((d->d_name[0] == '.' && d->d_name[1] == 0) ||
            (d->d_name[0] == '.' && d->d_name[1] == '.' && d->d_name[2] == 0)) {
            continue;
        }

        _tcsncpy(path, dirString, _CC_MAX_PATH_);
        path[_CC_MAX_PATH_ - 1] = 0;
        _tcscat(path, _CC_PATH_SEP_S_);
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
    tchar_t cwd[_CC_MAX_PATH_];
    _cc_get_current_directory(cwd, _cc_countof(cwd));
    printf("%s\n", cwd);
    if (argc == 2)
        print_dir(argv[1]);
    else
        print_dir(cwd);

    return 0;
}
