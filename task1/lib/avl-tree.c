#include "avl-tree.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

typedef avl_tree_node_t *(*node_allocator)(avl_tree_t *t, avl_tree_key_t k);
typedef void (*node_deallocator)(avl_tree_node_t *n);

static
avl_tree_node_t *node_simple_allocator(avl_tree_t *t, avl_tree_key_t k);
static
avl_tree_node_t *node_inplace_allocator(avl_tree_t *t, avl_tree_key_t k);

static const struct {
    node_allocator      allocator;
    node_deallocator    deallocator;
} node_operators[2] = {
    [false] = {
        .allocator      = node_simple_allocator,
        .deallocator    = (node_deallocator)free
    },
    [true] = {
        .allocator      = node_inplace_allocator,
        .deallocator    = (node_deallocator)free
    }
};

static
avl_tree_node_t *node_simple_allocator(avl_tree_t *t, avl_tree_key_t k) {
    avl_tree_node_t *n = malloc(sizeof(avl_tree_node_t));

    assert(n);

    n->height = 1;
    n->data = NULL;
    n->host = t;
    n->key = k;
    n->left = n->right = n->parent = NULL;

    return n;
}

static
avl_tree_node_t *node_inplace_allocator(avl_tree_t *t, avl_tree_key_t k) {
    avl_tree_node_t *n = malloc(sizeof(avl_tree_node_t) + t->node_data_size);

    assert(n);

    n->height = 1;
    n->data = n + 1;
    n->host = t;
    n->key = k;
    n->left = n->right = n->parent = NULL;

    return n;
}

static inline
int node_height(const avl_tree_node_t *n) {
    return n ? n->height : 0;
}

static inline
int node_balance_factor(const avl_tree_node_t *n) {
    return node_height(n->left) - node_height(n->right);
}

static inline
void node_fix_height(avl_tree_node_t *n) {
    unsigned char hl = node_height(n->left);
    unsigned char hr = node_height(n->right);

    n->height = (hl > hr ? hl : hr) + 1;
}

static inline
avl_tree_node_t *node_rotate_left(avl_tree_node_t *a) {
    avl_tree_node_t *b = a->right,
                    *C = b->left;

    b->parent = a->parent;
    a->parent = b;

    if (C)
        C->parent = a;

    b->left = a;
    a->right = C;

    node_fix_height(a);
    node_fix_height(b);

    return b;
}

static inline
avl_tree_node_t *node_rotate_right(avl_tree_node_t *a) {
    avl_tree_node_t *b = a->left,
                    *C = b->right;

    b->parent = a->parent;
    a->parent = b;

    if (C)
        C->parent = a;

    b->right = a;
    a->left = C;

    node_fix_height(a);
    node_fix_height(b);

    return b;
}

static inline
avl_tree_node_t *node_balance(avl_tree_node_t *n) {
    node_fix_height(n);

    switch (node_balance_factor(n)) {
        case 2:
            if (node_balance_factor(n->left) < 0)
                n->left = node_rotate_left(n->left);

            n = node_rotate_right(n);
            break;

        case -2:
            if (node_balance_factor(n->right) > 0)
                n->right = node_rotate_right(n->right);

            n = node_rotate_left(n);
            break;
    }

    return n;
}

static inline
avl_tree_node_t *node_insert(avl_tree_node_t *n,
                             avl_tree_node_t *parent,
                             avl_tree_t *t,
                             avl_tree_key_t k,
                             avl_tree_node_t **inserted) {
    if (!n) {
        n = node_operators[t->inplace].allocator(t, k);
        n->parent = parent;
        *inserted = n;

        return n;
    }

    if (k < n->key)
        n->left = node_insert(n->left, n, t, k, inserted);
    else
        n->right = node_insert(n->right, n, t, k, inserted);

    return node_balance(n);
}

static inline
avl_tree_node_t *node_insert_or_get(avl_tree_node_t *n,
                                    avl_tree_node_t *parent,
                                    avl_tree_t *t,
                                    avl_tree_key_t k,
                                    avl_tree_node_t **inserted,
                                    bool *node_inserted) {
    if (!n) {
        n = node_operators[t->inplace].allocator(t, k);
        n->parent = parent;
        *inserted = n;
        *node_inserted = true;

        return n;
    }

    if (k == n->key) {
        *inserted = n;
        *node_inserted = false;
        return n;
    }

    if (k < n->key)
        n->left = node_insert_or_get(n->left, n, t, k, inserted, node_inserted);
    else
        n->right = node_insert_or_get(n->right, n, t, k, inserted, node_inserted);

    return node_balance(n);
}

static inline
avl_tree_node_t *node_leftmost(avl_tree_node_t *n) {
    return n->left ? node_leftmost(n->left) : n;
}

static inline
avl_tree_node_t *node_rightmost(avl_tree_node_t *n) {
    return n->right ? node_rightmost(n->right) : n;
}

static inline
avl_tree_node_t *node_remove_min(avl_tree_node_t *n) {
    if (!n->left)
        return n->right;

    n->left = node_remove_min(n->left);

    return node_balance(n);
}

static inline
bool node_is_left(const avl_tree_node_t *left) {
    /* left = NULL             -> false
     * left->parent = NULL     -> false
     */
    return left && (left->parent && (left->parent->left == left));
}

static inline
bool node_is_right(const avl_tree_node_t *right) {
    /* right = NULL             -> false
     * right->parent = NULL     -> false
     */
    return right && (right->parent && (right->parent->right == right));
}

static inline
avl_tree_node_t *node_remove(avl_tree_node_t *n, avl_tree_key_t k,
                             bool *removed,
                             void **_data) {
    avl_tree_node_t copy, *min;
    bool n_is_left = false;

    if (!n)
        return NULL;

    if (k < n->key)
        n->left = node_remove(n->left, k, removed, _data);
    else if (k > n->key)
        n->right = node_remove(n->right, k, removed, _data);
    else {
        *removed = true;
        *_data = n->data;
        n_is_left = node_is_left(n);

        copy = *n;

        node_operators[n->host->inplace].deallocator(n);

        if (!copy.right) {
            if (copy.left)
                copy.left->parent = copy.parent;
            return copy.left;
        }

        min = node_leftmost(copy.right);

        min->right = node_remove_min(copy.right);
        min->left = copy.left;

        min->parent = copy.parent;

        if (min->parent) {
            if (n_is_left)
                min->parent->left = min;
            else
                min->parent->right = min;
        }

        if (min->left)
            min->left->parent = min;

        if (min->right)
            min->right->parent = min;

        return node_balance(min);
    }

    return node_balance(n);
}

static inline
avl_tree_node_t *node_get(avl_tree_node_t *n, avl_tree_key_t k) {
    if (!n)
        return n;

    if (k > n->key)
        return node_get(n->right, k);
    if (k < n->key)
        return node_get(n->left, k);

    return n;
}

static inline
avl_tree_node_t *node_next(avl_tree_node_t *n) {
    if (!n)
        return n;

    /* n != NULL */

    if (n->right)
        return node_leftmost(n->right);

    /* n->right = NULL */

    if (node_is_left(n))
        return n->parent;

    n = n->parent;

    while (n && !node_is_left(n))
        n = n->parent;

    if (n && !node_is_right(n))
        n = n->parent;

    return n;
}

static inline
avl_tree_node_t *node_prev(avl_tree_node_t *n) {
    if (!n)
        return n;

    /* n != NULL */

    if (n->left)
        return node_rightmost(n->left);

    /* n->left = NULL */

    if (node_is_right(n))
        return n->parent;

    n = n->parent;

    while (n && node_is_left(n))
        n = n->parent;

    if (n && !node_is_left(n))
        n = n->parent;

    return n;
}

static inline
void node_purge(avl_tree_node_t *n) {
    if (n->left)
        node_purge(n->left);

    if (n->right)
        node_purge(n->right);

    node_operators[n->host->inplace].deallocator(n);
}

/**************** API ****************/
void avl_tree_init(avl_tree_t *tree, bool inplace, size_t node_data_size) {
    assert(tree);

    tree->inplace = inplace;
    tree->node_data_size = node_data_size;
    tree->count = 0;
    tree->root = NULL;
}

avl_tree_node_t *avl_tree_get(avl_tree_t *t, avl_tree_key_t k) {
    avl_tree_node_t *n;

    assert(t);

    n = t->root;

    while (n && n->key != k)
        n = k > n->key ? n->right : n->left;

    return n;
}

avl_tree_node_t *avl_tree_add(avl_tree_t *t, avl_tree_key_t k) {
    avl_tree_node_t *n;
    assert(t);

    t->root = node_insert(t->root, t->root, t, k, &n);
    ++t->count;
    return n;
}

avl_tree_node_t *avl_tree_add_or_get(avl_tree_t *t, avl_tree_key_t k,
                                     bool *_inserted) {
    avl_tree_node_t *n;
    bool inserted = false;

    assert(t);
    assert(_inserted);

    t->root = node_insert_or_get(t->root, t->root, t, k, &n, &inserted);
    t->count += inserted;

    *_inserted = inserted;

    return n;
}

void *avl_tree_remove_get_data(avl_tree_t *t, avl_tree_key_t k) {
    void *d = NULL;
    bool removed = false;

    assert(t);

#if 0
    n = node_get(t->root, k);
    d = n->data;
#endif

    t->root = node_remove(t->root, k, &removed, &d);

    if (removed)
        --t->count;

    return d;
}

void avl_tree_remove(avl_tree_t *t, avl_tree_key_t k) {
    void *d;
    bool removed = false;

    assert(t);

    t->root = node_remove(t->root, k, &removed, &d);

    if (removed)
        --t->count;
}

avl_tree_node_t *avl_tree_node_next(avl_tree_node_t *n) {
    return node_next(n);
}

avl_tree_node_t *avl_tree_node_prev(avl_tree_node_t *n) {
    return node_prev(n);
}

avl_tree_node_t *avl_tree_node_min(avl_tree_node_t *n) {
    return n ? node_leftmost(n) : NULL;
}

avl_tree_node_t *avl_tree_node_max(avl_tree_node_t *n) {
    return n ? node_rightmost(n) : NULL;
}

void avl_tree_purge(avl_tree_t *tree) {
    assert(tree);

    if (tree->root)
        node_purge(tree->root);

    tree->count = 0;
    tree->root = NULL;
}
