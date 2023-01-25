#include "hash-map.h"
#include "avl-tree.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

void hash_map_init(hash_map_t *hm,
                   hash_function_t hasher,
                   hash_update_function_t hash_updater) {
    assert(hm);

    avl_tree_init(&hm->tree, true, sizeof(hash_map_node_t));

    hm->hasher = hasher;
    hm->hash_updater = hash_updater;
}

void hash_map_purge(hash_map_t *hm) {
    assert(hm);

    avl_tree_purge(&hm->tree);

    hm->hash_updater = NULL;
    hm->hasher = NULL;
}

size_t hash_map_size(hash_map_t *hm) {
    assert(hm);

    return hm->tree.count;
}

hash_map_node_t *hash_map_add(hash_map_t *hm, hash_t h) {
    avl_tree_node_t *atn;
    hash_map_node_t *hmn;

    assert(hm);

    atn = avl_tree_add(&hm->tree, h);

    hmn = atn->data;
    hmn->host = hm;
    hmn->hash = h;
    hmn->tree_node = atn;

    list_init(&hmn->data_list, true, sizeof(hash_map_node_data_t));

    return hmn;
}

hash_map_node_t *hash_map_add_or_get(hash_map_t *hm, hash_t h) {
    avl_tree_node_t *atn;
    hash_map_node_t *hmn;
    bool inserted = false;

    assert(hm);

    atn = avl_tree_add_or_get(&hm->tree, h, &inserted);

    hmn = atn->data;

    if (inserted) {
        hmn->host = hm;
        hmn->hash = h;
        hmn->tree_node = atn;

        list_init(&hmn->data_list, true, sizeof(hash_map_node_data_t));
    }

    return hmn;
}

hash_map_node_t *hash_map_get(hash_map_t *hm, hash_t h) {
    avl_tree_node_t *atn;
    hash_map_node_t *hmn;

    assert(hm);

    atn = avl_tree_get(&hm->tree, h);

    hmn = atn ? atn->data : NULL;

    return hmn;
}

void hash_map_remove(hash_map_t *hm, hash_t h) {
    hash_map_node_t *hmn;

    assert(hm);

    hmn = avl_tree_remove_get_data(&hm->tree, h);

    list_purge(&hmn->data_list);
}

hash_map_node_t *hash_map_next(hash_map_t *hm, hash_map_node_t *hmn) {
    avl_tree_node_t *atn_next;
    hash_map_node_t *next;

    assert(hm);

    if (!hmn)
        return NULL;

    atn_next = avl_tree_node_next(hmn->tree_node);
    next = atn_next ? atn_next->data : NULL;

    return next;
}

hash_map_node_t *hash_map_prev(hash_map_t *hm, hash_map_node_t *hmn) {
    avl_tree_node_t *atn_prev;
    hash_map_node_t *prev;

    assert(hm);

    if (!hmn)
        return NULL;

    atn_prev = avl_tree_node_prev(hmn->tree_node);
    prev = atn_prev ? atn_prev->data : NULL;

    return prev;
}

hash_map_node_t *hash_map_begin(hash_map_t *hm) {
    avl_tree_node_t *atn_begin;
    hash_map_node_t *begin;

    assert(hm);

    atn_begin = avl_tree_node_min(hm->tree.root);

    begin = atn_begin ? atn_begin->data : NULL;

    return begin;
}

hash_map_node_t *hash_map_end(hash_map_t *hm) {
    avl_tree_node_t *atn_end;
    hash_map_node_t *end;

    assert(hm);

    atn_end = avl_tree_node_max(hm->tree.root);

    end = atn_end ? atn_end->data : NULL;

    return end;
}

size_t hash_map_node_size(hash_map_node_t *hmn) {
    assert(hmn);

    return hmn->data_list.count;
}

hash_map_node_element_t *hash_map_node_add(hash_map_node_t *hmn,
                                           hash_map_node_data_t hmnd) {
    list_element_t *le;

    assert(hmn);

    le = list_append(&hmn->data_list);

    if (!le || !le->data)
        return NULL;

    memcpy(le->data, &hmnd, sizeof(hmnd));
    return (hash_map_node_element_t *)le;
}

void hash_map_node_remove(hash_map_node_t *hmn, hash_map_node_element_t *el) {
    assert(hmn);

    if (!el)
        return;

    assert(&hmn->data_list == el->host);

    list_remove_and_advance(&hmn->data_list, (list_element_t *)el);
}

hash_map_node_element_t *hash_map_node_next(hash_map_node_t *hmn,
                                            hash_map_node_element_t *el) {
    assert(hmn);

    if (!el)
        return NULL;

    assert(&hmn->data_list == el->host);

    return (hash_map_node_element_t *)list_next(&hmn->data_list,
                                                (list_element_t *)el);
}

hash_map_node_element_t *hash_map_node_prev(hash_map_node_t *hmn,
                                            hash_map_node_element_t *el) {
    assert(hmn);

    if (!el)
        return NULL;

    assert(&hmn->data_list == el->host);

    return (hash_map_node_element_t *)list_prev(&hmn->data_list,
                                                (list_element_t *)el);
}

hash_map_node_element_t *hash_map_node_begin(hash_map_node_t *hmn) {
    assert(hmn);

    return (hash_map_node_element_t *)list_begin(&hmn->data_list);
}

hash_map_node_element_t *hash_map_node_end(hash_map_node_t *hmn) {
    assert(hmn);

    return (hash_map_node_element_t *)list_end(&hmn->data_list);
}

void *hash_map_node_element_get_data(const hash_map_node_element_t *el) {
    return (void *)(el ? el->data : NULL);
}
