# ARRAY

> 可变长数组

宏
---------------

| 宏 | 描述  |
| :--------------- |:----------------------- |
| [_cc_array_for_each](#ARRAY) | 遍历数组元素 |
| [_cc_array_count](#ARRAY) | 获取数组元素个数 |

函数
---------------

| 函数 | 描述  |
| :--------------- |:----------------------- |
| [_cc_create_array](#ARRAY) | 创建数组 |
| [_cc_destroy_array](#ARRAY) | 销毁数组 |
| [_cc_array_alloc](#ARRAY) | 初始化数组和长度 |
| [_cc_array_free](#ARRAY) | 释放数组 |
| [_cc_array_cleanup](#ARRAY) | 清空数组中的元素 |
| [_cc_array_find](#ARRAY) | 查找数组 |
| [_cc_array_append](#ARRAY) | 合并数组 |
| [_cc_array_insert](#ARRAY) | 向数组中插入一个元素 |
| [_cc_array_remove](#ARRAY) | 向数组中删除一个元素 |
| [_cc_array_push](#ARRAY) | 向数组中插入尾部插入一个元素 |
| [_cc_array_pop](#ARRAY) | 向数组中插入尾部移出一个元素 |
| [_cc_array_begin](#ARRAY) | 数组头一个元素 |
| [_cc_array_end](#ARRAY) | 数组尾一个元素 |
| [_cc_array_get_count](#ARRAY) | 获取数组元素个数 |


## ARRAY 例子
```c
_cc_array_t *arr;
int *find_val = NULL;
uint32_t i = 0;

arr = _cc_create_array(15);
for (i = 0;  i < 10; i++) {
    int *a = CC_MALLOC(int);
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
```