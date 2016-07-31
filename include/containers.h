#ifndef _CONTAINERS_H_
# define _CONTAINERS_H_

/** \file containers.h
 * Containers/collections library:
 * buffer, vector, array, list, queue, stack
 */

# include <stddef.h>
# include <stdbool.h>

enum buffer_policy {
    bp_shrinkable          = 0,
    bp_non_shrinkable      = 1,
    bp_economic            = 2,
    buffer_policy_max   = 3
};

/** Dumb buffer with shrinkability selection
 */
typedef struct buffer {
    enum buffer_policy pol;
    /** user data */
    void *data;
    /** size requested by user */
    size_t user_size;
    /** really allocated size */
    size_t real_size;
} buffer_t;

/** A vector with dynamic size
 */
typedef struct vector {
    buffer_t data;
    size_t element_size;
    size_t count;
} vector_t;

/** A doubly linked list
 */
typedef struct list list_t;
typedef struct list_element {
    list_t *host;
    struct list_element *prev;
    struct list_element *next;
    void *data;
} list_element_t;

struct list {
    list_element_t *front;
    list_element_t *back;
    size_t count;
    bool inplace;
    size_t element_size;
};

/**** buffer operations ****/
void buffer_init(buffer_t *b,
                 size_t size,
                 enum buffer_policy pol);

bool buffer_realloc(buffer_t *b, size_t newsize);
void buffer_deinit(buffer_t *b);

/**** vector operations ****/
void vector_init(vector_t *v, size_t size, size_t count);
void vector_deinit(vector_t *v);
void vector_remove(vector_t *v, size_t idx);
void vector_remove_range(vector_t *v, size_t from, size_t count);
void *vector_insert(vector_t *v, size_t idx);
void *vector_append(vector_t *v);
void *vector_prepend(vector_t *v);
void *vector_begin(vector_t *v);
void *vector_end(vector_t *v);
void *vector_get(vector_t *v, size_t idx);
void *vector_next(vector_t *v, void *d);
void *vector_prev(vector_t *v, void *d);

/**** list operations ****/
void list_init(list_t *l, bool inplace, size_t size);
size_t list_size(const list_t *l);
list_element_t *list_prepend(list_t *l);
list_element_t *list_append(list_t *l);
list_element_t *list_add_after(list_t *l, list_element_t *el);
list_element_t *list_add_before(list_t *l, list_element_t *el);
list_element_t *list_remove_and_advance(list_t *l, list_element_t *el);
void list_purge(list_t *l);
list_element_t *list_begin(list_t *l);
list_element_t *list_end(list_t *l);
list_element_t *list_next(list_t *l, list_element_t *el);
list_element_t *list_prev(list_t *l, list_element_t *el);

#endif
