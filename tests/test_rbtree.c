#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libcc/rbtree.h>

// 数据节点结构
struct data_node {
    _cc_rb_t node;
    int key;
};

// 递归遍历
void rbtree_traverse_recursive(_cc_rb_t *node, void (*cb)(_cc_rb_t *)) {
    if (node->left) rbtree_traverse_recursive(node->left, cb);
    cb(node);
    if (node->right) rbtree_traverse_recursive(node->right, cb);
}

// 迭代遍历
void rbtree_traverse_iterative(_cc_rbtree_t *root, void (*cb)(_cc_rb_t *)) {
    _cc_rb_t *node;
    for (node = _cc_rbtree_first(root); node; node = _cc_rb_next(node)) {
        cb(node);
    }
}
// 优化迭代遍历函数
void rbtree_iterative_traverse(_cc_rbtree_t *root, void (*cb)(_cc_rb_t *)) {
    _cc_rb_t *stack[1000], *node = root->rb_node;
    int top = -1;
    while (node || top >= 0) {
        if (node) {
            stack[++top] = node;
            node = node->left;
        } else {
            node = stack[top--];
            cb(node); // 处理节点
            node = node->right;
        }
    }
}
// 回调函数
void print_node(_cc_rb_t *node) {
    struct data_node *data = _cc_upcast(node, struct data_node, node);
    //printf("%d ", data->key);
    if (data == 0) {
        printf("%d ", data->key);
    }
}

_CC_API_PUBLIC(bool_t) rbtree_push(_cc_rbtree_t *root, struct data_node *data) {
    int32_t result = 0;
    struct data_node *self;
    _cc_rb_t **node = &(root->rb_node), *parent = NULL;

    while (*node) {
        self = _cc_upcast(*node, struct data_node, node);
        result = self->key - data->key;

        parent = *node;

        if (result < 0) {
            node = &((*node)->left);
        } else if (result > 0) {
            node = &((*node)->right);
        } else {
            return false;
        }
    }

    _cc_rbtree_insert(root, &data->node, parent, node);

    return true;
}

int main() {
    _cc_rbtree_t mytree = { NULL };
    struct data_node *nodes[1000000];

    // 插入10000个节点
    for (int i = 0; i < 1000000; i++) {
        nodes[i] = malloc(sizeof(struct data_node));
        nodes[i]->key = i;
        rbtree_push(&mytree,nodes[i]);
    }

    // 测试递归遍历性能
    clock_t start = clock();
    rbtree_traverse_recursive(mytree.rb_node, print_node);
    clock_t end = clock();
    printf("\n递归遍历耗时: %.6f秒\n", (double)(end - start) / CLOCKS_PER_SEC);

    // 测试迭代遍历性能
    start = clock();
    rbtree_traverse_iterative(&mytree, print_node);
    end = clock();
    printf("迭代遍历耗时: %.6f秒\n", (double)(end - start) / CLOCKS_PER_SEC);

    // 测试迭代遍历性能
    start = clock();
    rbtree_iterative_traverse(&mytree, print_node);
    end = clock();
    printf("优化迭代遍历: %.6f秒\n", (double)(end - start) / CLOCKS_PER_SEC);
    return 0;
}
