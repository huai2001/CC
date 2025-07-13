#include <stdio.h>
#include <libcc.h>

#define _FILE_FILLS_BUF_SIZE_ 1024

typedef struct _filters_file {
    bool_t isfile;
    size_t length;
    tchar_t *path
    _cc_rbtree_iterator_t node;
}_filters_file_t;

typedef struct _filters_name {
    size_t length;
    tchar_t *path;
}_filters_name_t;

static struct {
    bool_t s;
    bool_t o;
    bool_t r;
    _cc_rbtree_t filters; 
} _rd = {0};


static void _safe_unlink(tchar_t *pf) {
    /*byte_t buf[_FILE_FILLS_BUF_SIZE_] = {0};
    _cc_file_t *fp = _cc_open_file(pf, _T("wb"));
    if (fp) {
        int64_t size = _cc_file_size(fp);
        while (size > _FILE_FILLS_BUF_SIZE_) {
            _cc_file_write(fp, buf, sizeof(byte_t), _FILE_FILLS_BUF_SIZE_);
            size -= _FILE_FILLS_BUF_SIZE_;
        }

        if (size > 0) {
            _cc_file_write(fp, buf, sizeof(byte_t), (size_t)size);
        }

        _cc_file_close(fp);
        _cc_unlink(pf);
    }*/
}

static void _delete_file(tchar_t *path) {
    if (_rd.s) {
        _safe_unlink(path);
    } else {
        //_cc_unlink(path);
    }

    if (_rd.o) {
        _tprintf("rd file:%s\n", path);
    }
}
static int32_t filters_get(_cc_rbtree_iterator_t *node, pvoid_t args) {
    _filters_name_t *f = (_filters_name_t*)args;
    _filters_file_t *ff = _cc_upcast(node, _filters_file_t, lnk);
    return strincmp(ff->path, f->path, ff->length);
}

static bool_t filler_file_list(tchar_t *name, int32_t namlen) {
    _filters_name_t f;
    f.path = name;
    f.length = namlen;

    if (_cc_rbtree_get(&_rd.filters, &f, filters_get)) {
        return true;
    }
    return false;
}

bool_t _depth_delete_file(tchar_t *root) {
    tchar_t path[_CC_MAX_PATH_] = {0};
    struct dirent *d = nullptr;

    DIR *dir = opendir(root);
    if (dir == nullptr) {
        return false;
    }
    
    while((d = readdir(dir)) != nullptr) {
        if ((d->d_name[0]=='.' && d->d_name[1] == 0) ||
            (d->d_name[0]=='.' && d->d_name[1] == '.' && d->d_name[2] == 0)) {
            continue;
        }

        _tcsncpy(path, root, _CC_MAX_PATH_);
        path[_CC_MAX_PATH_ - 1] = 0;

        _tcscat(path, _CC_SLASH_S_);
        _tcscat(path, d->d_name);
        
        if (d->d_type == DT_DIR) {
            if (_rd.r) {
                _depth_delete_file(path);
                //_rmdir(path);
                if (_rd.o) {
                    _tprintf("rd dir:%s\n", path);
                }
            }
        } else {
            _delete_file(path);
        }
    }
    closedir(dir);
    return true;
}

void _delete(tchar_t *root) {
    if (_cc_isdir(root)) {
        _depth_delete_file(root);
    } else {
        _delete_file(root);
    }
}

int _tmain (int argc, tchar_t * const argv[]) {
    tchar_t *arg = nullptr;
    int i = 2;
    tchar_t cwd[_CC_MAX_PATH_];
    arg = argv[1];

    _rd.s = false;
    _rd.o = false;
    _rd.r = false;

    if (*arg == '-') {
        arg++;
        while (*arg) {
            switch(*arg) {
                case 'r':
                    _rd.r = true;
                break;
                case 'o':
                    _rd.o = true;
                break;
                case 's':
                    _rd.s = true;
                    break;
                case 'v': {
                    puts("Version 1.0.0.0\n");
                    return 0;
                }
                case 'h':{
                    puts("rd: usage: rd [-r|-o|-s|-v|-h] [dir] [...]\n");
                    return 0;
                }
            }
            arg++;
        }
        i = 2;
    } else {
        i = 1;
    }
    
    _cc_get_current_directory(cwd, _CC_MAX_PATH_);

    for (i; i < argc; i++) {
        _delete(argv[i]);
    }

    return 0;
}
