#include "libcc/list.h"
#include "libcc/platform.h"
#include <assert.h>
#include <stdio.h>

void test_list_iterator_empty() {
    _cc_list_t head;
    _cc_list_cleanup(&head);
    assert(_cc_list_empty(&head) == true);

    _cc_list_t node;
    _cc_list_cleanup(&node);
    _cc_list_push_front(&head, &node);
    assert(_cc_list_empty(&head) == false);
}

void test_list_iterator_insert() {
    _cc_list_t head;
    _cc_list_cleanup(&head);

    _cc_list_t node1, node2;
    _cc_list_cleanup(&node1);
    _cc_list_cleanup(&node2);

    _cc_list_insert(&node1, &head, &head);
    assert(head.next == &node1);
    assert(head.prev == &node1);
    assert(node1.next == &head);
    assert(node1.prev == &head);

    _cc_list_insert(&node2, &node1, &head);
    assert(node1.next == &node2);
    assert(node2.prev == &node1);
    assert(node2.next == &head);
    assert(head.prev == &node2);
}

void test_list_iterator_delete() {
    _cc_list_t head;
    _cc_list_cleanup(&head);

    _cc_list_t node1, node2;
    _cc_list_cleanup(&node1);
    _cc_list_cleanup(&node2);

    _cc_list_insert(&node1, &head, &head);
    _cc_list_insert(&node2, &node1, &head);

    _cc_list_delete(&node1, &head);
    assert(node1.next == &head);
    assert(head.prev == &node1);
}

void test_list_iterator_push_pop() {
    _cc_list_t head;
    _cc_list_cleanup(&head);

    _cc_list_t node1, node2;
    _cc_list_cleanup(&node1);
    _cc_list_cleanup(&node2);

    _cc_list_push_front(&head, &node1);
    assert(head.next == &node1);
    assert(node1.prev == &head);

    _cc_list_push_back(&head, &node2);
    assert(head.prev == &node2);
    assert(node2.next == &head);

    _cc_list_t *popped = _cc_list_pop_front(&head);
    assert(popped == &node1);
    assert(head.next == &node2);

    popped = _cc_list_pop_back(&head);
    assert(popped == &node2);
    assert(_cc_list_empty(&head) == true);
}

void test_list_iterator_swap() {
    _cc_list_t head1, head2;
    _cc_list_cleanup(&head1);
    _cc_list_cleanup(&head2);

    _cc_list_t node1, node2;
    _cc_list_cleanup(&node1);
    _cc_list_cleanup(&node2);

    _cc_list_push_front(&head1, &node1);
    _cc_list_push_front(&head2, &node2);

    _cc_list_swap(&head1, &node2);
    assert(head1.next == &node2);
    assert(head2.next == &node1);
}

int main() {
    test_list_iterator_empty();
    test_list_iterator_insert();
    test_list_iterator_delete();
    test_list_iterator_push_pop();
    test_list_iterator_swap();

    printf("All tests passed!\n");
    return 0;
}