#include <stdio.h>

#include <libcc/alloc.h>
#include <libcc/array.h>
#include <libcc/string.h>

int main (int argc, char * const argv[]) {
    _cc_array_t *arr;
    int *find_val = nullptr;
    uint32_t i = 0;
    
    arr = _cc_create_array(15);
    for (i = 0;  i < 10; i++) {
        int *a = _CC_MALLOC(int);
        *a = i + 100;
        _cc_array_push(arr, a);
    }
    
    find_val = (int*)_cc_array_find(arr,5);
    if(find_val)
        _tprintf(_T("find item: %d\n"), *(int*)find_val);
    
    _cc_array_for_each(int, val, key, arr, {
        _tprintf(_T("%d , %d\n"), *(int*)val, key);
    });
    
    for (i = 0;  i < 5; i++) {
        void *a = _cc_array_remove(arr, i);
        _cc_free(a);
    }
    
    _tprintf(_T("print array data deleted! \n"));
    /**/
    _cc_array_for_each(int, val, key, arr, {
        _tprintf(_T("%d , %d\n"), *(int*)val, key);
        _cc_free(val);
    });
    
    _cc_destroy_array(&arr);
    
    system("pause");
    return 0;
}