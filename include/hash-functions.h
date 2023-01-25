#ifndef _HASH_FUNCTIONS_H_
# define _HASH_FUNCTIONS_H_

# include <stdint.h>
# include <stddef.h>

typedef int64_t hash_t;

typedef hash_t (*hash_function_t)(const void *data, size_t size);
typedef hash_t (*hash_update_function_t)(hash_t hash,
                                         const void *data, size_t size);

hash_t hash_pearson(const void *data, size_t size);
hash_t hash_update_pearson(hash_t hash, const void *data, size_t size);

#endif
