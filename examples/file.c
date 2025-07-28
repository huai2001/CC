/* System dependent filesystem routines */
#include <libcc/generic.h>
#include <libcc/alloc.h>
#include <libcc/dirent.h>


int main(int argc, char const *argv[]) {
    tchar_t cwd[_CC_MAX_PATH_] = {0};
    tchar_t file_path_name[_CC_MAX_PATH_ * 2] = {0};
    byte_t data[1024];
    _cc_file_t *r = nullptr, *w = nullptr;

    _cc_get_base_path(cwd, _cc_countof(cwd));
    _sntprintf(file_path_name, _cc_countof(file_path_name), _T("%s/test.c"), cwd);
    
    w = _cc_open_file(file_path_name, "w");
    if(w){
        _cc_file_write(w, "testes\n", sizeof(char_t), 7);
        _cc_file_close(w);
    }
    
    r = _cc_open_file(file_path_name, "r");
    if (r) {
        size_t byte_read = 0;
        printf("fileSize:%lld\n", _cc_file_size(r));
        while((byte_read = _cc_file_read(r, data, sizeof(tchar_t), _cc_countof(data))) > 0) {
            //printf("read %ld\n", byte_read);
            printf("%.*s", (int)byte_read,data);
        }
        _cc_file_close(r);
    }
    
    system("pause");
    return 0;
}
