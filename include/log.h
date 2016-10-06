#ifndef _LOG_H_
# define _LOG_H_

# include <stdio.h>

# define LOG_LEVEL_DEBUG    "[debug]  "
# define LOG_LEVEL_INFO     "[info]   "
# define LOG_LEVEL_WARN     "[warn]   "
# define LOG_LEVEL_ERROR    "[error]  "
# define LOG_LEVEL_FATAL    "[fatal]  "

# define LOG(level, msg, ...)                     \
do {                                              \
    fprintf(stderr, level "[%10d] ", time(NULL)); \
    fprintf(stderr, msg, __VA_ARGS__);            \
} while (0)

# define LOG_MSG(level, msg)                      \
do {                                              \
    fprintf(stderr, level "[%10d] ", time(NULL)); \
    fprintf(stderr, msg);                         \
} while (0)

# define LOG_LN()           fprintf(stderr, "\n")

#endif /* _LOG_H_ */