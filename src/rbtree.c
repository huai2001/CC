/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
 * All rights reserved.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:

 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/
#include <libcc/rbtree.h>

/*
 * red-black trees properties:  http://en.wikipedia.org/wiki/Rbtree
 *
 *  1) A node is either red or black
 *  2) The root is black
 *  3) All leaves (nullptr) are black
 *  4) Both children of every red node are black
 *  5) Every simple path from root to leaves contains the same number
 *     of black nodes.
 *
 *  4 and 5 give the O(log n) guarantee, since 4 implies you cannot have two
 *  consecutive red nodes in a path and every red node is therefore followed by
 *  a black. So if B is the number of black nodes on every simple path (as per
 *  5), then the longest possible path due to 4 is 2B.
 *
 *  We shall indicate color with case, where black nodes are uppercase and red
 *  nodes will be lowercase. Unknown color nodes shall be drawn as red within
 *  parentheses and have some accompanying text comment.
 */

#define _rb_color(r) ((r)->parent_color & 1)
#define _rb_is_red(r) (!_rb_color(r))
#define _rb_is_black(r) (_rb_color(r))
#define _rb_parent(r) ((_cc_rbtree_iterator_t *)((r)->parent_color & ~3))
#define _rb_write_once(x, v) (x) = (v)

/* 'empty' nodes are nodes that are known not to be inserted in an rbtree */
#define RB_EMPTY_NODE(node) ((node)->parent_color == (uintptr_t)(node))

_CC_FORCE_INLINE_ void _rb_change_child(_cc_rbtree_iterator_t *old_iter, _cc_rbtree_iterator_t *new_iter, _cc_rbtree_iterator_t *parent, _cc_rbtree_t *root) {
    if (parent) {
        if (parent->left == old_iter) {
            _rb_write_once(parent->left, new_iter);
        } else {
            _rb_write_once(parent->right, new_iter);
        }
    } else {
        _rb_write_once(root->rb_node, new_iter);
    }
}

_CC_FORCE_INLINE_ void _rb_set_black(_cc_rbtree_iterator_t *rb) {
    rb->parent_color |= _CC_RB_BLACK_;
}

_CC_FORCE_INLINE_ _cc_rbtree_iterator_t* _rb_red_parent(_cc_rbtree_iterator_t *red) {
    return (_cc_rbtree_iterator_t *)red->parent_color;
}

_CC_FORCE_INLINE_ void _rb_set_parent(_cc_rbtree_iterator_t *rb, _cc_rbtree_iterator_t *p) {
    rb->parent_color = _rb_color(rb) | (uintptr_t)p;
}

_CC_FORCE_INLINE_ void _rb_set_parent_color(_cc_rbtree_iterator_t *rb, _cc_rbtree_iterator_t *p, int color) {
    rb->parent_color = (uintptr_t)p | color;
}

/*
 * Helper function for rotations:
 * - old's parent and color get assigned to new
 * - old gets assigned new as a parent and 'color' as a color.
 */
_CC_API_PRIVATE(void) _rb_rotate_set_parents(_cc_rbtree_iterator_t *old, _cc_rbtree_iterator_t *new,
                                              _cc_rbtree_t *root, int color) {
    _cc_rbtree_iterator_t *parent = _rb_parent(old);
    new->parent_color = old->parent_color;
    _rb_set_parent_color(old, new, color);
    _rb_change_child(old, new, parent, root);
}

// insert a node into the tree at the right place, rejig ptrs as needed
_CC_API_PUBLIC(void) _cc_rbtree_insert_color(_cc_rbtree_t *root, _cc_rbtree_iterator_t *node) {
    _cc_rbtree_iterator_t *parent = _rb_red_parent(node), *gparent, *tmp;

    while (true) {
        /*
         * Loop invariant: node is red.
         */
        if (_cc_unlikely(!parent)) {
            /*
             * The inserted node is root. Either this is the
             * first node, or we recursed at Case 1 below and
             * are no longer violating 4).
             */
            _rb_set_parent_color(node, nullptr, _CC_RB_BLACK_);
            break;
        }

        /*
         * If there is a black parent, we are done.
         * Otherwise, take some corrective action as,
         * per 4), we don't want a red root or two
         * consecutive red nodes.
         */
        if (_rb_is_black(parent)) {
            break;
        }

        gparent = _rb_red_parent(parent);

        tmp = gparent->right;
        /* parent == gparent->left */
        if (parent != tmp) {
            if (tmp && _rb_is_red(tmp)) {
                /*
                 * Case 1 - node's uncle is red (color flips).
                 *
                 *       G            g
                 *      / \          / \
                 *     p   u  -->   P   U
                 *    /            /
                 *   n            n
                 *
                 * However, since g's parent might be red, and
                 * 4) does not allow this, we need to recurse
                 * at g.
                 */
                _rb_set_parent_color(tmp, gparent, _CC_RB_BLACK_);
                _rb_set_parent_color(parent, gparent, _CC_RB_BLACK_);
                node = gparent;
                parent = _rb_parent(node);
                _rb_set_parent_color(node, parent, _CC_RB_RED_);
                continue;
            }

            tmp = parent->right;
            if (node == tmp) {
                /*
                 * Case 2 - node's uncle is black and node is
                 * the parent's right child (left rotate at parent).
                 *
                 *      G             G
                 *     / \           / \
                 *    p   U  -->    n   U
                 *     \           /
                 *      n         p
                 *
                 * This still leaves us in violation of 4), the
                 * continuation into Case 3 will fix that.
                 */
                tmp = node->left;
                _rb_write_once(parent->right, tmp);
                _rb_write_once(node->left, parent);
                if (tmp) {
                    _rb_set_parent_color(tmp, parent, _CC_RB_BLACK_);
                }
                _rb_set_parent_color(parent, node, _CC_RB_RED_);
                parent = node;
                tmp = node->right;
            }

            /*
             * Case 3 - node's uncle is black and node is
             * the parent's left child (right rotate at gparent).
             *
             *        G           P
             *       / \         / \
             *      p   U  -->  n   g
             *     /                 \
             *    n                   U
             */
            /*gparent->left == parent->right */
            _rb_write_once(gparent->left, tmp);
            _rb_write_once(parent->right, gparent);
            if (tmp) {
                _rb_set_parent_color(tmp, gparent, _CC_RB_BLACK_);
            }
            _rb_rotate_set_parents(gparent, parent, root, _CC_RB_RED_);
            break;
        } else {
            tmp = gparent->left;
            if (tmp && _rb_is_red(tmp)) {
                /* Case 1 - color flips */
                _rb_set_parent_color(tmp, gparent, _CC_RB_BLACK_);
                _rb_set_parent_color(parent, gparent, _CC_RB_BLACK_);
                node = gparent;
                parent = _rb_parent(node);
                _rb_set_parent_color(node, parent, _CC_RB_RED_);
                continue;
            }

            tmp = parent->left;
            if (node == tmp) {
                /* Case 2 - right rotate at parent */
                tmp = node->right;
                _rb_write_once(parent->left, tmp);
                _rb_write_once(node->right, parent);
                if (tmp) {
                    _rb_set_parent_color(tmp, parent, _CC_RB_BLACK_);
                }
                _rb_set_parent_color(parent, node, _CC_RB_RED_);
                parent = node;
                tmp = node->left;
            }

            /* Case 3 - left rotate at gparent */
            /* gparent->right == parent->left */
            _rb_write_once(gparent->right, tmp);
            _rb_write_once(parent->left, gparent);
            if (tmp) {
                _rb_set_parent_color(tmp, gparent, _CC_RB_BLACK_);
            }
            _rb_rotate_set_parents(gparent, parent, root, _CC_RB_RED_);
            break;
        }
    }
}

_CC_API_PRIVATE(void) _rb_erase_color(_cc_rbtree_iterator_t *parent, _cc_rbtree_t *root) {
    _cc_rbtree_iterator_t *node = nullptr, *sibling, *tmp1, *tmp2;

    while (true) {
        /*
         * Loop invariants:
         * - node is black (or nullptr on first iteration)
         * - node is not the root (parent is not nullptr)
         * - All leaf paths going through parent and node have a
         *   black node count that is 1 lower than other leaf paths.
         */
        sibling = parent->right;
        if (node != sibling) {
            if (_rb_is_red(sibling)) {
                /*
                 * Case 1 - left rotate at parent
                 *
                 *     P               S
                 *    / \             / \
                 *   N   s    -->    p   Sr
                 *      / \         / \
                 *     Sl  Sr      N   Sl
                 */
                tmp1 = sibling->left;
                _rb_write_once(parent->right, tmp1);
                _rb_write_once(sibling->left, parent);
                _rb_set_parent_color(tmp1, parent, _CC_RB_BLACK_);
                _rb_rotate_set_parents(parent, sibling, root, _CC_RB_RED_);
                sibling = tmp1;
            }
            tmp1 = sibling->right;
            if (!tmp1 || _rb_is_black(tmp1)) {
                tmp2 = sibling->left;
                if (!tmp2 || _rb_is_black(tmp2)) {
                    /*
                     * Case 2 - sibling color flip
                     * (p could be either color here)
                     *
                     *    (p)           (p)
                     *    / \           / \
                     *   N   S    -->  N   s
                     *      / \           / \
                     *     Sl  Sr        Sl  Sr
                     *
                     * This leaves us violating 5) which
                     * can be fixed by flipping p to black
                     * if it was red, or by recursing at p.
                     * p is red when coming from Case 1.
                     */
                    _rb_set_parent_color(sibling, parent, _CC_RB_RED_);
                    if (_rb_is_red(parent)) {
                        _rb_set_black(parent);
                    } else {
                        node = parent;
                        parent = _rb_parent(node);
                        if (parent) {
                            continue;
                        }
                    }
                    break;
                }
                /*
                 * Case 3 - right rotate at sibling
                 * (p could be either color here)
                 *
                 *   (p)           (p)
                 *   / \           / \
                 *  N   S    -->  N   sl
                 *     / \             \
                 *    sl  Sr            S
                 *                       \
                 *                        Sr
                 *
                 * Note: p might be red, and then both
                 * p and sl are red after rotation(which
                 * breaks property 4). This is fixed in
                 * Case 4 (in _rb_rotate_set_parents()
                 *         which set sl the color of p
                 *         and set p _CC_RB_BLACK_)
                 *
                 *   (p)            (sl)
                 *   / \            /  \
                 *  N   sl   -->   P    S
                 *       \        /      \
                 *        S      N        Sr
                 *         \
                 *          Sr
                 */
                tmp1 = tmp2->right;
                _rb_write_once(sibling->left, tmp1);
                _rb_write_once(tmp2->right, sibling);
                _rb_write_once(parent->right, tmp2);
                if (tmp1) {
                    _rb_set_parent_color(tmp1, sibling, _CC_RB_BLACK_);
                }
                tmp1 = sibling;
                sibling = tmp2;
            }
            /*
             * Case 4 - left rotate at parent + color flips
             * (p and sl could be either color here.
             *  After rotation, p becomes black, s acquires
             *  p's color, and sl keeps its color)
             *
             *      (p)             (s)
             *      / \             / \
             *     N   S     -->   P   Sr
             *        / \         / \
             *      (sl) sr      N  (sl)
             */
            tmp2 = sibling->left;
            _rb_write_once(parent->right, tmp2);
            _rb_write_once(sibling->left, parent);
            _rb_set_parent_color(tmp1, sibling, _CC_RB_BLACK_);
            if (tmp2) {
                _rb_set_parent(tmp2, parent);
            }
            _rb_rotate_set_parents(parent, sibling, root, _CC_RB_BLACK_);
            break;
        } else {
            sibling = parent->left;
            if (_rb_is_red(sibling)) {
                /* Case 1 - right rotate at parent */
                tmp1 = sibling->right;
                _rb_write_once(parent->left, tmp1);
                _rb_write_once(sibling->right, parent);
                _rb_set_parent_color(tmp1, parent, _CC_RB_BLACK_);
                _rb_rotate_set_parents(parent, sibling, root, _CC_RB_RED_);
                sibling = tmp1;
            }
            tmp1 = sibling->left;
            if (!tmp1 || _rb_is_black(tmp1)) {
                tmp2 = sibling->right;
                if (!tmp2 || _rb_is_black(tmp2)) {
                    /* Case 2 - sibling color flip */
                    _rb_set_parent_color(sibling, parent, _CC_RB_RED_);
                    if (_rb_is_red(parent)) {
                        _rb_set_black(parent);
                    } else {
                        node = parent;
                        parent = _rb_parent(node);
                        if (parent)
                            continue;
                    }
                    break;
                }
                /* Case 3 - left rotate at sibling */
                tmp1 = tmp2->left;
                _rb_write_once(sibling->right, tmp1);
                _rb_write_once(tmp2->left, sibling);
                _rb_write_once(parent->left, tmp2);
                if (tmp1) {
                    _rb_set_parent_color(tmp1, sibling, _CC_RB_BLACK_);
                }
                tmp1 = sibling;
                sibling = tmp2;
            }
            /* Case 4 - right rotate at parent + color flips */
            tmp2 = sibling->right;
            _rb_write_once(parent->left, tmp2);
            _rb_write_once(sibling->right, parent);
            _rb_set_parent_color(tmp1, sibling, _CC_RB_BLACK_);
            if (tmp2) {
                _rb_set_parent(tmp2, parent);
            }
            _rb_rotate_set_parents(parent, sibling, root, _CC_RB_BLACK_);
            break;
        }
    }
}

_CC_API_PRIVATE(_cc_rbtree_iterator_t*) _rb_erase(_cc_rbtree_iterator_t *node, _cc_rbtree_t *root) {
    _cc_rbtree_iterator_t *child = node->right;
    _cc_rbtree_iterator_t *tmp = node->left;
    _cc_rbtree_iterator_t *parent, *rebalance;

    if (!tmp) {
        /*
         * Case 1: node to erase has no more than 1 child (easy!)
         *
         * Note that if there is one child it must be red due to 5)
         * and node must be black due to 4). We adjust colors locally
         * so as to bypass _rb_erase_color() later on.
         */
        parent = _rb_parent(node);
        _rb_change_child(node, child, parent, root);
        if (child) {
            child->parent_color = node->parent_color;
            rebalance = nullptr;
        } else {
            rebalance = _rb_is_black(node) ? parent : nullptr;
        }
    } else if (!child) {
        /* Still case 1, but this time the child is node->left */
        tmp->parent_color = node->parent_color;
        parent = _rb_parent(node);
        _rb_change_child(node, tmp, parent, root);
        rebalance = nullptr;
    } else {
        _cc_rbtree_iterator_t *successor = child, *child2;

        tmp = child->left;
        if (!tmp) {
            /*
             * Case 2: node's successor is its right child
             *
             *    (n)          (s)
             *    / \          / \
             *  (x) (s)  ->  (x) (c)
             *        \
             *        (c)
             */
            parent = successor;
            child2 = successor->right;
        } else {
            /*
             * Case 3: node's successor is leftmost under
             * node's right child subtree
             *
             *    (n)          (s)
             *    / \          / \
             *  (x) (y)  ->  (x) (y)
             *      /            /
             *    (p)          (p)
             *    /            /
             *  (s)          (c)
             *    \
             *    (c)
             */
            do {
                parent = successor;
                successor = tmp;
                tmp = tmp->left;
            } while (tmp);
            child2 = successor->right;
            _rb_write_once(parent->left, child2);
            _rb_write_once(successor->right, child);
            _rb_set_parent(child, successor);
        }

        tmp = node->left;
        _rb_write_once(successor->left, tmp);
        _rb_set_parent(tmp, successor);

        tmp = _rb_parent(node);
        _rb_change_child(node, successor, tmp, root);

        if (child2) {
            successor->parent_color = node->parent_color;
            _rb_set_parent_color(child2, parent, _CC_RB_BLACK_);
            rebalance = nullptr;
        } else {
            rebalance = _rb_is_black(successor) ? parent : nullptr;
            successor->parent_color = node->parent_color;
        }
    }

    return rebalance;
}

_CC_API_PUBLIC(void) _cc_rbtree_erase(_cc_rbtree_t *root, _cc_rbtree_iterator_t *node) {
    _cc_rbtree_iterator_t *rebalance;
    rebalance = _rb_erase(node, root);
    if (rebalance) {
        _rb_erase_color(rebalance, root);
    }
}

/*
 * This function returns the first node (in sort order) of the tree.
 */
_CC_API_PUBLIC(_cc_rbtree_iterator_t*) _cc_rbtree_first(const _cc_rbtree_t *root) {
    _cc_rbtree_iterator_t *n;

    n = root->rb_node;
    if (!n) {
        return nullptr;
    }

    while (n->left) {
        n = n->left;
    }
    return n;
}

_CC_API_PUBLIC(_cc_rbtree_iterator_t*) _cc_rbtree_last(const _cc_rbtree_t *root) {
    _cc_rbtree_iterator_t *n;

    n = root->rb_node;
    if (!n) {
        return nullptr;
    }

    while (n->right) {
        n = n->right;
    }
    return n;
}

_CC_API_PUBLIC(_cc_rbtree_iterator_t*) _cc_rbtree_next(const _cc_rbtree_iterator_t *node) {
    _cc_rbtree_iterator_t *parent;

    if (RB_EMPTY_NODE(node)) {
        return nullptr;
    }

    /*
     * If we have a right-hand child, go down and then left as far
     * as we can.
     */
    if (node->right) {
        node = node->right;
        while (node->left) {
            node = node->left;
        }
        return (_cc_rbtree_iterator_t *)node;
    }

    /*
     * No right-hand children. Everything down and left is smaller than us,
     * so any 'next' node must be in the general direction of our parent.
     * Go up the tree; any time the ancestor is a right-hand child of its
     * parent, keep going up. First time it's a left-hand child of its
     * parent, said parent is our 'next' node.
     */
    while ((parent = _rb_parent(node)) && node == parent->right) {
        node = parent;
    }
    return parent;
}

_CC_API_PUBLIC(_cc_rbtree_iterator_t*) _cc_rbtree_prev(const _cc_rbtree_iterator_t *node) {
    _cc_rbtree_iterator_t *parent;

    if (RB_EMPTY_NODE(node))
        return nullptr;

    /*
     * If we have a left-hand child, go down and then right as far
     * as we can.
     */
    if (node->left) {
        node = node->left;
        while (node->right) {
            node = node->right;
        }
        return (_cc_rbtree_iterator_t *)node;
    }

    /*
     * No left-hand children. Go up till we find an ancestor which
     * is a right-hand child of its parent.
     */
    while ((parent = _rb_parent(node)) && node == parent->left) {
        node = parent;
    }

    return parent;
}

_CC_API_PUBLIC(void) _cc_rbtree_replace_node(_cc_rbtree_t *root, _cc_rbtree_iterator_t *victim, _cc_rbtree_iterator_t *new_iter) {
    _cc_rbtree_iterator_t *parent = _rb_parent(victim);

    /* Copy the pointers/colour from the victim to the replacement */
    *new_iter = *victim;

    /* Set the surrounding nodes to point to the replacement */
    if (victim->left) {
        _rb_set_parent(victim->left, new_iter);
    }

    if (victim->right) {
        _rb_set_parent(victim->right, new_iter);
    }

    _rb_change_child(victim, new_iter, parent, root);
}

_CC_API_PUBLIC(_cc_rbtree_iterator_t*) _cc_rbtree_get(const _cc_rbtree_t *root, pvoid_t args,
                                      int32_t (*func)(_cc_rbtree_iterator_t *, pvoid_t)) {
    int32_t result = 0;
    _cc_rbtree_iterator_t *node = root->rb_node;

    while (node) {
        result = func(node, args);
        if (result < 0) {
            node = node->left;
        } else if (result > 0) {
            node = node->right;
        } else {
            return node;
        }
    }

    return nullptr;
}

_CC_API_PUBLIC(bool_t) _cc_rbtree_push(_cc_rbtree_t *root, _cc_rbtree_iterator_t *data,
                       int32_t (*func)(_cc_rbtree_iterator_t *, _cc_rbtree_iterator_t *)) {
    int32_t result = 0;
    _cc_rbtree_iterator_t **node = &(root->rb_node), *parent = nullptr;

    while (*node) {
        result = func(*node, data);

        parent = *node;

        if (result < 0) {
            node = &((*node)->left);
        } else if (result > 0) {
            node = &((*node)->right);
        } else {
            return false;
        }
    }

    _cc_rbtree_insert(root, data, parent, node);

    return true;
}

_CC_API_PUBLIC(void) _cc_rbtree_traverse(_cc_rbtree_iterator_t *node, void (*_func)(_cc_rbtree_iterator_t *, pvoid_t), pvoid_t args) {
    if (node->left) {
        _cc_rbtree_traverse(node->left, _func, args);
    }

    if (node->right) {
        _cc_rbtree_traverse(node->right, _func, args);
    }

    _func(node, args);
}

static void _free_rbtree_traverse(_cc_rbtree_iterator_t *node, void (*_func)(_cc_rbtree_iterator_t *)) {
    if (node->left) {
        _free_rbtree_traverse(node->left, _func);
    }

    if (node->right) {
        _free_rbtree_traverse(node->right, _func);
    }

    _func(node);
}

_CC_API_PUBLIC(void) _cc_rbtree_destroy(_cc_rbtree_t *root, void (*_func)(_cc_rbtree_iterator_t *)) {
    _cc_assert(_func != nullptr);
    if (!_func || !root->rb_node) {
        return;
    }
    _free_rbtree_traverse(root->rb_node, _func);
    root->rb_node = nullptr;
}
