#ifndef _HASH_MAP_H
# define _HASH_MAP_H_

/** \file hash-map.h
 * Hash map library.
 * Hash map is AVL tree with each node as a list of data.
 * Data is a tuple: pointer with data size.
 *
 * \c hash_map_t is the tree.
 * \c hash_map_node_t is tree node data and contains user data
 *                    list with equal hashes.
 * \c hash_map_node_data_t is user data element.
 *
 * \c hash_map_node_t::data_list is inplace list of \c hash_map_node_data_t.
 */

# include "hash-functions.h"
# include "avl-tree.h"
# include "containers.h"

# include <stdint.h>
# include <stddef.h>

typedef struct hash_map {
    avl_tree_t tree;
    hash_function_t hasher;
    hash_update_function_t hash_updater;
} hash_map_t;

typedef struct hash_map_node {
    hash_map_t *host;
    avl_tree_node_t *tree_node;
    hash_t hash;
    list_t data_list;
} hash_map_node_t;

typedef struct hash_map_node_data {
    void *data;
    size_t size;
} hash_map_node_data_t;

typedef list_element_t hash_map_node_element_t;

void hash_map_init(hash_map_t *hm,
                   hash_function_t hasher,
                   hash_update_function_t hash_updater);
void hash_map_purge(hash_map_t *hm);
size_t hash_map_size(hash_map_t *hm);
hash_map_node_t *hash_map_add(hash_map_t *hm, hash_t h);
hash_map_node_t *hash_map_add_or_get(hash_map_t *hm, hash_t h);
hash_map_node_t *hash_map_get(hash_map_t *hm, hash_t h);
void hash_map_remove(hash_map_t *hm, hash_t h);
hash_map_node_t *hash_map_next(hash_map_t *hm, hash_map_node_t *hmn);
hash_map_node_t *hash_map_prev(hash_map_t *hm, hash_map_node_t *hmn);
hash_map_node_t *hash_map_begin(hash_map_t *hm);
hash_map_node_t *hash_map_end(hash_map_t *hm);

size_t hash_map_node_size(hash_map_node_t *hmn);
hash_map_node_element_t *hash_map_node_add(hash_map_node_t *hmn,
                                           hash_map_node_data_t hmnd);
void hash_map_node_remove(hash_map_node_t *hmn,
                          hash_map_node_element_t *el);
hash_map_node_element_t *hash_map_node_next(hash_map_node_t *hmn,
                                            hash_map_node_element_t *el);
hash_map_node_element_t *hash_map_node_prev(hash_map_node_t *hmn,
                                            hash_map_node_element_t *el);
hash_map_node_element_t *hash_map_node_begin(hash_map_node_t *hmn);
hash_map_node_element_t *hash_map_node_end(hash_map_node_t *hmn);

void *hash_map_node_element_get_data(const hash_map_node_element_t *el);

#endif
