#ifndef _AVL_TREE_H_
# define _AVL_TREE_H_

/** \file avl-tree.h
 * AVL tree library
 */

# include <stdbool.h>
# include <stddef.h>
# include <stdint.h>

struct avl_tree;
typedef struct avl_tree avl_tree_t;

struct avl_tree_node;
typedef struct avl_tree_node avl_tree_node_t;

typedef int64_t avl_tree_key_t;

struct avl_tree_node {
    /*! host tree */
    avl_tree_t *host;
    /*! less-than node */
    avl_tree_node_t *left;
    /*! more-than node */
    avl_tree_node_t *right;
    /*! parent node */
    avl_tree_node_t *parent;
    unsigned char height;
    avl_tree_key_t key;
    void *data;
};

struct avl_tree {
    size_t count;
    bool inplace;
    size_t node_data_size;
    avl_tree_node_t *root;
};

void avl_tree_init(avl_tree_t *tree, bool inplace, size_t node_data_size);
avl_tree_node_t *avl_tree_get(avl_tree_t *t, avl_tree_key_t k);
avl_tree_node_t *avl_tree_add(avl_tree_t *t, avl_tree_key_t k);
avl_tree_node_t *avl_tree_add_or_get(avl_tree_t *t, avl_tree_key_t k,
                                     bool *_inserted);
void *avl_tree_remove_get_data(avl_tree_t *t, avl_tree_key_t k);
void avl_tree_remove(avl_tree_t *t, avl_tree_key_t k);
/* void avl_tree_remove_direct(avl_tree_t *t, avl_tree_node_t *n); */
avl_tree_node_t *avl_tree_node_next(avl_tree_node_t *n);
avl_tree_node_t *avl_tree_node_prev(avl_tree_node_t *n);
avl_tree_node_t *avl_tree_node_min(avl_tree_node_t *n);
avl_tree_node_t *avl_tree_node_max(avl_tree_node_t *n);
void avl_tree_purge(avl_tree_t *tree);

#endif