#ifndef _MAP_H
#define _MAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libcc.h>

typedef struct _map _map_t;

#define KEY_MAX_LENGTH (256)

#define TESTES 0
#define TESTES_STRING 0

struct _map
{
#if TESTES_STRING
    char key_string[KEY_MAX_LENGTH];
#endif
    uint32_t number;
#if TESTES == 0
    _cc_rbtree_iterator_t node;
#endif
};

#if TESTES == 0
#if TESTES_STRING
_map_t *get(_cc_rbtree_t *root, char* key) {
#else
_map_t *get(_cc_rbtree_t *root, uint32_t key) {
#endif
   _cc_rbtree_iterator_t *node = root->rb_node;
   while (node) {
        _map_t *item = _cc_upcast(node, _map_t, node);

        //compare between the key with the keys in map
#if TESTES_STRING
       int result = strcmp(key, item->key_string);
#else
       int result = (key - item->number);
#endif
        if (result < 0) {
            node = node->left;
        }else if (result > 0) {
            node = node->right;
        }else {
            return item;
        }
   }
   return NULL;
}
#if TESTES_STRING
int put(_cc_rbtree_t *root, char* key, _map_t* data) {
#else
int put(_cc_rbtree_t *root, uint32_t key, _map_t* data) {
#endif
    _cc_rbtree_iterator_t **node = &(root->rb_node), *parent = NULL;
    while (*node) {
        _map_t *item = _cc_upcast(*node, _map_t, node);
#if TESTES_STRING
        int result = strcmp(key, item->key_string);
#else
        int result = (key - item->number);
#endif
        parent = *node;

        if (result < 0) {
            node = &((*node)->left);
        } else if (result > 0) {
            node = &((*node)->right);
        } else {
            return 0;
        }
    }
    _cc_rbtree_insert(root, &data->node, parent, node);

    return 1;
}
_map_t *map_first(_cc_rbtree_t *tree) {
    _cc_rbtree_iterator_t *node = _cc_rbtree_first(tree);
    return (_cc_rbtree_entry(node, _map_t, node));
}

_map_t *map_next(_cc_rbtree_iterator_t *node) {
    _cc_rbtree_iterator_t *next =  _cc_rbtree_next(node);
    return _cc_rbtree_entry(next, _map_t, node);
}
#if TESTES_STRING
_map_t *map_erase(_cc_rbtree_t *root, char* key) {
#else
_map_t *map_erase(_cc_rbtree_t *root, uint32_t key) {
#endif
    _map_t *m = get(root, key);
    if (m) {
        _cc_rbtree_erase(root, &m->node);
    }
    return m;
}

void map_free(_map_t *node){
        _cc_free(node);
        node = NULL;
}
void _map_free(_cc_rbtree_iterator_t *node) {
    _map_t *data;
    if (node->left) {
        _map_free(node->left);
    }
    if (node->right) {
        _map_free(node->right);
    }
    data = _cc_upcast(node, _map_t, node);
    _cc_free(data);
}

void map_destroy(_cc_rbtree_t *root) {
    /*_map_t *data;
    _cc_rbtree_for_each(node, root, {
        data = _cc_upcast(node, _map_t, node);
        _cc_rbtree_erase(root, node);
        _cc_free(data);
    });*/
    if (root->rb_node) {
        _map_free(root->rb_node);
    }
}
#endif

#endif  //_MAP_H

/* vim: set ts=4 sw=4 sts=4 tw=100 */
