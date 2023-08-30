/* System dependent filesystem routines */
#include <cc/core.h>
#include <cc/alloc.h>
#include <cc/dirent.h>


int main(int argc, char const *argv[]) {
    tchar_t cwd[_CC_MAX_PATH_] = {0};
    byte_t data[1024];
    _cc_file_t *r = NULL, *w = NULL;

    _cc_get_module_directory("test.c", cwd, _cc_countof(cwd));
    
    w = _cc_open_file(cwd, "w");
    if(w){
        _cc_file_write(w, "testes\n", sizeof(char_t), 7);
        _cc_file_close(w);
    }
    
    r = _cc_open_file(cwd, "r");
    if (r) {
        size_t byte_read = 0;
        printf("fileSize:%lld\n", _cc_file_size(r));
        while((byte_read = _cc_file_read(r, data, sizeof(tchar_t), _cc_countof(data))) > 0) {
            //printf("read %ld\n", byte_read);
            printf("%.*s", (int)byte_read,data);
        }
        _cc_file_close(r);
    }
    
    return 0;
}
