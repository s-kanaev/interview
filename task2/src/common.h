#ifndef _COMMON_H_
# define _COMMON_H_

# include <stddef.h>

# define DIR         "/tmp/driver"
# define DIR_LEN     (sizeof(DIR) - 1)

# define MAX_DIGITS  (10)

# define SUFFIX      "drv"
# define SUFFIX_LEN  (sizeof(SUFFIX) - 1)

# define DOT         "."
# define DOT_LEN     (sizeof(DOT) - 1)

# define SLASH       "/"
# define SLASH_LEN   (sizeof(SLASH) - 1)

int allocate_unix_socket(const char *name, size_t name_len);

#endif