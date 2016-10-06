#ifndef _COMMON_H_
# define _COMMON_H_

# include "io-service.h"

# include <stddef.h>

# define BASE_DIR       "/tmp/driver"
# define BASE_DIR_LEN   (sizeof(BASE_DIR) - 1)

# define MAX_DIGITS  (10)

# define SUFFIX      "drv"
# define SUFFIX_LEN  (sizeof(SUFFIX) - 1)

# define DOT         "."
# define DOT_LEN     (sizeof(DOT) - 1)

# define SLASH       "/"
# define SLASH_LEN   (sizeof(SLASH) - 1)

int allocate_unix_socket(const char *name, size_t name_len);

/**
 * Post a job to wait for sigterm, sigint through \c io_serivce_t instance.
 * \param [in] iosvc io service instance
 * \return \c true on success, \c false on failure
 *
 * The job will stop io service instance without waiting
 * for pending jobs.
 */
bool wait_for_sigterm_sigint(io_service_t *iosvc);

#endif