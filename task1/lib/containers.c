#include "containers.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#if 0
static const int _PRE_FRONT = 1;
static const int _POST_BACK = 2;
static const void const *PRE_FRONT_PTR = &_PRE_FRONT;
static const void const *POST_BACK_PTR = &_POST_BACK;

#define PRE_FRONT ((void *)PRE_FRONT_PTR)
#define POST_BACK ((void *)POST_BACK_PTR)
#endif

typedef bool (*reallocer_func)(buffer_t *b, size_t newsize);

static
bool realloc_shrinkable(buffer_t *b, size_t newsize) {
    void *d;
    assert(b);
    d = realloc(b->data, newsize);

    if (d) {
        b->user_size = b->real_size = newsize;
        b->data = d;
    }

    return !!d;
}

static
bool realloc_nonshrinkable(buffer_t *b, size_t newsize) {
    void *d;

    assert(b);

    if (newsize < b->real_size) {
        b->user_size = newsize;
        return true;
    }

    d = realloc(b->data, newsize);

    if (d) {
        b->user_size = newsize;
        b->real_size = newsize;
        b->data = d;
    }

    return !!d;
}

static
bool realloc_economic(buffer_t *b, size_t newsize) {
    assert(b);

    size_t lesser = b->real_size / 2;
    size_t greater = b->real_size * 2;
    size_t rs;
    void *d;

    if (newsize < b->real_size / 4)
        rs = lesser;
    else if (newsize > b->real_size) {
        if (newsize < greater)
            rs = greater;
        else
            rs = newsize;
    }
    else {
        b->user_size = newsize;
        return true;
    }

    d = realloc(b->data, rs);

    if (d) {
        b->user_size = newsize;
        b->real_size = rs;
        b->data = d;
    }

    return !!d;
}

static const reallocer_func reallocer[buffer_policy_max] = {
    [bp_shrinkable]      = realloc_shrinkable,
    [bp_non_shrinkable]  = realloc_nonshrinkable,
    [bp_economic]        = realloc_economic
};

/***************************** BUFFER *****************************/
void buffer_init(buffer_t *b, size_t size, enum buffer_policy pol) {
    assert(b && pol < buffer_policy_max);

    b->pol = pol;
    b->real_size = b->user_size = size;
    b->data = malloc(b->user_size);
}

bool buffer_realloc(buffer_t *b, size_t newsize) {
    assert(b);
    return reallocer[b->pol](b, newsize);
}

void buffer_deinit(buffer_t *b) {
    assert(b);

    if (b->data)
        free(b->data);

    b->user_size = b->real_size = 0;
}

/***************************** VECTOR *****************************/
void vector_init(vector_t *v, size_t size, size_t count) {
    assert(v);
    v->element_size = size;
    v->count = count;
    buffer_init(&v->data, size * count, bp_economic);
}

void vector_remove(vector_t *v, size_t idx) {
    size_t shifting;
    void *newpos;
    bool realloced;

    assert(v);
    assert(v->count > idx);

    -- v->count;

    newpos = v->data.data + idx * v->element_size;
    shifting = (v->count - idx) * v->element_size;
    memmove(
        newpos,
        newpos + v->element_size,
        shifting
    );

    realloced = buffer_realloc(&v->data, v->count * v->element_size);
    assert(realloced);
}

void vector_remove_range(vector_t *v, size_t from, size_t count) {
    size_t shifting;
    void *newpos;
    bool realloced;

    if (!count)
        return;

    assert(v);
    assert(v->count > from && v->count > from + count - 1);

    v->count -= count;
    newpos = v->data.data + from * v->element_size;
    shifting = (v->count - from) * v->element_size;

    memmove(
        newpos,
        newpos + count * v->element_size,
        shifting
    );

    realloced = buffer_realloc(&v->data, v->count * v->element_size);
    assert(realloced);
}

void *vector_insert(vector_t *v, size_t idx) {
    size_t shifting;
    void *newelement;
    void *newpos;
    bool realloced;

    assert(v);
    assert(idx <= v->count);

    newelement = v->data.data + v->element_size * idx;
    newpos = newelement + v->element_size;
    shifting = v->element_size * (v->count - idx);

    ++v->count;

    realloced = buffer_realloc(&v->data, v->element_size * (v->count + 1));

    assert(realloced);

    memmove(newpos, newelement, shifting);

    return newelement;
}

void *vector_append(vector_t *v) {
    assert(v);

    size_t filled = v->element_size * v->count;
    void *d;
    bool realloc_ret = buffer_realloc(&v->data, filled + v->element_size);

    assert(realloc_ret);

    d = v->data.data + filled;
    ++v->count;

    return d;
}

void *vector_prepend(vector_t *v) {
    assert(v);

    size_t filled = v->element_size * v->count;
    void *d;
    bool realloc_ret = buffer_realloc(&v->data, filled + v->element_size);

    assert(realloc_ret);

    memmove(
        v->data.data + v->element_size,
        v->data.data,
        filled
    );

    d = v->data.data;
    ++v->count;

    return d;
}

void *vector_begin(vector_t *v) {
    assert(v);
    return v->data.data;
}

void *vector_end(vector_t *v) {
    assert(v);
    return v->data.data + v->count * v->element_size;
}

void *vector_get(vector_t *v, size_t idx) {
    assert(v);
    assert(v->count > idx);

    return v->data.data + idx * v->element_size;
}

void *vector_next(vector_t *v, void *d) {
    assert(v);
    assert(!(d < v->data.data));
    assert(d < v->data.data + v->count * v->element_size);
    assert((d - v->data.data) % v->element_size == 0);

    return d + v->element_size;
}

void *vector_prev(vector_t *v, void *d) {
    assert(v);
    assert(!(d < v->data.data));
    assert(d < v->data.data + v->count * v->element_size);
    assert((d - v->data.data) % v->element_size == 0);

    return d == v->data.data ? d : d - v->element_size;
}

void vector_deinit(vector_t *v) {
    assert(v);
    buffer_deinit(&v->data);
    v->count = 0;
    v->element_size = 0;
}

/***************************** LIST *****************************/
typedef list_element_t *(*list_el_allocator)(list_t *l);
typedef void (*list_el_deallocator)(list_element_t *el);

static
list_element_t *alloc_lel_inplace(list_t *l) {
    list_element_t *le = malloc(sizeof(list_element_t) + l->element_size);
    le->data = le + 1;
    le->host = l;
    le->next = le->prev = NULL;
    return le;
}

static
list_element_t *alloc_lel_simple(list_t *l) {
    list_element_t *le = malloc(sizeof(list_element_t));
    le->data = NULL;
    le->host = l;
    le->next = le->prev = NULL;
    return le;
}

static const struct {
    list_el_allocator   allocator;
    list_el_deallocator deallocator;
} list_el_alloc_dealloc[2] = {
    [true]  = {
        .allocator      = alloc_lel_inplace,
        .deallocator    = (list_el_deallocator)free
    },
    [false] = {
        .allocator      = alloc_lel_simple,
        .deallocator    = (list_el_deallocator)free
    }
};

void list_init(list_t *l, bool inplace, size_t size) {
    assert(l);
    assert(!inplace || (inplace && size));

    l->front = l->back = NULL;

    l->element_size = size;
    l->inplace = inplace;

    l->count = 0;
}

size_t list_size(const list_t *l) {
    assert(l);
    return l->count;
}

list_element_t *list_prepend(list_t *l) {
    list_element_t *el;

    assert(l);

    el = list_el_alloc_dealloc[l->inplace].allocator(l);

    el->prev = NULL;
    el->next = l->front;
    l->front = el;

    if (el->next)
        el->next->prev = el;

    if (!l->back)
        l->back = el;

    ++ l->count;
    return el;
}

list_element_t *list_append(list_t *l) {
    list_element_t *el;

    assert(l);

    el = list_el_alloc_dealloc[l->inplace].allocator(l);

    el->prev = l->back;
    el->next = NULL;
    l->back = el;

    if (el->prev)
        el->prev->next = el;

    if (!l->front)
        l->front = el;

    ++ l->count;
    return el;
}

list_element_t *list_add_after(list_t *l, list_element_t *el) {
    list_element_t *newel;

    assert(l);
    assert(!el || el->host == l);

    if (!el)
        return list_prepend(l);

    newel = list_el_alloc_dealloc[l->inplace].allocator(l);

    newel->prev = el;

    if (newel->prev) {
        newel->next = el->next;

        newel->prev->next = newel;

        if (newel->next)
            newel->next->prev = newel;
    }

    if (!l->front)
        l->front = newel;

    if (!l->back || l->back == el)
        l->back = newel;

    ++ l->count;
    return newel;
}

list_element_t *list_add_before(list_t *l, list_element_t *el) {
    list_element_t *newel;

    assert(l);
    assert(!el || el->host == l);

    if (!el)
        return list_append(l);

    newel = list_el_alloc_dealloc[l->inplace].allocator(l);

    newel->next = el;

    if (newel->next) {
        newel->prev = el->prev;

        newel->next->prev = newel;

        if (newel->prev)
            newel->prev->next = newel;
    }

    if (!l->front || l->front == el)
        l->front = newel;

    if (!l->back)
        l->back = newel;

    ++ l->count;
    return newel;
}

list_element_t *list_remove_and_advance(list_t *l, list_element_t *el) {
    list_element_t *prev = NULL, *next = NULL;
    assert(l);
    assert(!el || l == el->host);

    if (!l->count)
        return NULL;

    if (el) {
        prev = el->prev;
        next = el->next;
        list_el_alloc_dealloc[l->inplace].deallocator(el);
    }

    if (prev)
        prev->next = next;

    if (next)
        next->prev = prev;

    -- l->count;

    if (el == l->front)
        l->front = next;

    if (el == l->back)
        l->back = prev;

    return next;
}

void list_purge(list_t *l) {
    list_element_t *el;

    assert(l);

    if (!l->count) {
        l->front = NULL;
        l->back = NULL;
        return;
    }

    for (el = l->front; el; el = el->next)
        list_el_alloc_dealloc[l->inplace].deallocator(el);

    l->front = NULL;
    l->back = NULL;

    l->count = 0;
}

list_element_t *list_begin(list_t *l) {
    assert(l);
    return l->front;
}

list_element_t *list_end(list_t *l) {
    assert(l);
    return l->back;
}

list_element_t *list_next(list_t *l, list_element_t *el) {
    list_element_t *next;
    assert(l);
    assert(!el || el->host == l);

    next = (!el
            ? l->front : el->next);
    return next;
}

list_element_t *list_prev(list_t *l, list_element_t *el) {
    list_element_t *prev;
    assert(l);
    assert(!el || el->host == l);

    prev = (!el
            ? l->back : el->prev);
    return prev;
}
