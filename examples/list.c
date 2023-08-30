#include <stdio.h>

#include <cc/queue.h>
#include <cc/list.h>
#include <cc/alloc.h>
#include <cc/logger.h>
#include <cc/time.h>

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
        _cc_queue_sync_push(&head2, &ab2[i].base);
    }
    ab1[N].i = N;
    ab2[N].i = N;
    _cc_list_iterator_pop_front(&head1);
    _cc_list_iterator_pop_back(&head1);
    _cc_list_iterator_remove(&ab1[51].base);
    
    _cc_list_iterator_push_front(&ab1[55].base,&ab1[N].base);
    
    pos = _cc_list_iterator_index(&head1, -5);
    t = _cc_upcast(pos, list1_t, base);
    printf("%d\n",t->i);
    
    printf("list:\n");
    for (pos = head1.next; pos != &(head1); pos = pos->next) {
        t = _cc_upcast(pos, list1_t, base);
        printf("%d, ", t->i);
    }
    putchar('\n');
    
    printf("list:\n");
    _cc_list_iterator_sort(&head1,_list_sort);
    for (pos = head1.next; pos != &(head1); pos = pos->next) {
        t = _cc_upcast(pos, list1_t, base);
        printf("%d, ", t->i);
    }
    putchar('\n');
    
    _cc_queue_sync_pop(&head2);
    _cc_queue_iterator_pop(&head2);
    _cc_queue_sync_push(&head2,&ab2[N].base);
    
    pos2 = _cc_queue_iterator_index(&head2, 5);
    t2 = _cc_upcast(pos2, list2_t, base);
    printf("%d\n",t2->i);
    printf("queue:\n");
    for (pos2 = head2.next; pos2 != &(head2); pos2 = pos2->next) {
        list2_t *t2 = _cc_upcast(pos2, list2_t, base);
        printf("%d, ", t2->i);
    }
    i = 0;
    printf("\npop all:\n");
    while (_cc_list_iterator_pop(&head1)) {
        printf("%d, ", ++i);
    }

    while (getchar() != 'q') _cc_sleep(100);

    return 0;
}
