# double link

> 双向链表

链表结构体
---------------

```c
/*双向链表*/
struct _cc_list_iterator
{
    _cc_list_iterator_t *prev; //向上指针
    _cc_list_iterator_t *next; //向下指针
};

/*单向链表*/
struct _cc_queue_iterator
{
    _cc_queue_iterator_t *next; //向下指针
};
```

链表函数
---------------

| 函数 | 描述  |
| :--------------- |:----------------------- |
| [_cc_list_iterator_for_each_next](#doubly_linked) | 安全型向下遍历链表 |
| [_cc_list_iterator_for_each_prev](#doubly_linked) | 安全型向上遍历链表 |
| [_cc_list_iterator_for_each](#doubly_linked) | 同 _cc_list_iterator_for_each_next |
| [_cc_list_iterator_for_prev](#_cc_list_iterator_for) | 向下遍历链表 |
| [_cc_list_iterator_for_next](#_cc_list_iterator_for) | 向上遍历链表 |
| [_cc_list_iterator_for](#_cc_list_iterator_for) | 同 _cc_list_iterator_for_next |
| [_cc_list_iterator_push_front](#doubly_linked) | 双向链表头部加入一个对象  |
| [_cc_list_iterator_push_back](#doubly_linked) | 双向链表尾部加入一个对象 
| [_cc_list_iterator_pop_back](#doubly_linked) | 双向链表尾部移出一个对象 |
| [_cc_list_iterator_pop_front](#doubly_linked) | 双向链表头部移出一个对象 | |
| [_cc_list_iterator_push](#doubly_linked) | 同 _cc_list_iterator_push_back |
| [_cc_list_iterator_pop](#doubly_linked) | 同 _cc_list_iterator_pop_front |
| [_cc_list_iterator_sort | 双向链表排序 |
| [_cc_list_iterator_append](#doubly_linked) | 合并两个链表 |
| [_cc_list_iterator_remove](#doubly_linked) | 删除链表中的一个对象 |
| [_cc_list_iterator_swap](#doubly_linked) | 两个链表交换一个对象 |
| [_cc_list_iterator_empty](#doubly_linked) | 是否为空链表 |
| [_cc_list_iterator_cleanup](#_cc_list_iterator_cleanup) | 清空链表 (清空前先释放资源) |


链表函数
---------------

| 函数 | 描述  |
| :--------------- |:----------------------- |
| [_cc_queue_iterator_for_each](#doubly_linked) | 安全型向下遍历链表 |
| [_cc_queue_iterator_for](#doubly_linked) | 向下遍历链表 |
| [_cc_queue_iterator_push](#doubly_linked) | 向链表头部加入一个对象  |
| [_cc_queue_iterator_pop](#doubly_linked) | 向链表头部移出一个对象 |
| [_cc_queue_iterator_empty](#doubly_linked) | 是否为空链表 |
| [_cc_queue_iterator_cleanup](#doubly_linked) | 清空链表 (清空前先释放资源) |

## doubly_linked
```c

#define N 100//1024 * 1024 * 2

typedef struct list1
{
    _cc_list_iterator_t base;
    int i;
} list1_t;

typedef struct list2 {
    _cc_queue_iterator_t base;
    int i;
}list2_t;


_cc_list_iterator_t head1;
_cc_queue_iterator_t head2;

int32_t _list_sort(const _cc_list_iterator_t *l, const _cc_list_iterator_t *r) {
    const list1_t* _l = _cc_upcast(l, list1_t, base);
    const list1_t* _r = _cc_upcast(r, list1_t, base);

    return _l->i - _r->i;
}

int main (int argc, char * const argv[]) {
    int i = 0;
    list1_t *t;
    list2_t *t2;
    list1_t ab1[N+1];
    list2_t ab2[N+1];
    _cc_list_iterator_t *pos;
    _cc_queue_iterator_t *pos2;
    bzero(ab1, sizeof(ab1));
    bzero(ab2, sizeof(ab2));
    
    _cc_list_iterator_cleanup(&head1);
    _cc_queue_iterator_cleanup(&head2);
    
    for (i = 0;  i < N; i++) {
        ab1[i].i = i;
        ab2[i].i = i;
        _cc_list_iterator_push_front(&head1,&ab1[i].base);
        _cc_queue_iterator_push(&head2, &ab2[i].base);
    }
    ab1[N].i = N;
    _cc_list_iterator_pop_front(&head1);
    _cc_list_iterator_pop_back(&head1);
    _cc_list_iterator_remove(&ab1[51].base);
    
    _cc_list_iterator_push_front(&ab1[55].base,&ab1[N].base);
    
    pos = _cc_list_iterator_index(&head1, -5);
    t = _cc_upcast(pos, list1_t, base);
    printf("%d\n",t->i);
    
    for (pos = head1.next; pos != &(head1); pos = pos->next) {
        t = _cc_upcast(pos, list1_t, base);
        printf("%d, ", t->i);
    }
    putchar('\n');

    _cc_list_iterator_sort(&head1,_list_sort);
    for (pos = head1.next; pos != &(head1); pos = pos->next) {
        t = _cc_upcast(pos, list1_t, base);
        printf("%d, ", t->i);
    }
    putchar('\n');
    
    _cc_queue_iterator_pop(&head2);
    
    pos2 = _cc_queue_iterator_index(&head2, 5);
    t2 = _cc_upcast(pos2, list2_t, base);
    printf("%d\n",t2->i);
    for (pos2 = head2.next; pos2 != &(head2); pos2 = pos2->next) {
        list2_t *t2 = _cc_upcast(pos2, list2_t, base);
        printf("%d, ", t2->i);
    }
    while (getchar() != 'q') _cc_sleep(100);

    return 0;
}
```