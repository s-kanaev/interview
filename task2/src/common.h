#ifndef _COMMON_H_
# define _COMMON_H_

# include <stddef.h>

int allocate_unix_socket(const char *name, size_t name_len);

#endif