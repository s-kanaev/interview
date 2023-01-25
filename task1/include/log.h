#ifndef _LOG_H_
# define _LOG_H_

# include <stdio.h>

# define LOG_LEVEL_DEBUG    "[debug]  "
# define LOG_LEVEL_INFO     "[info]   "
# define LOG_LEVEL_WARN     "[warn]   "
# define LOG_LEVEL_ERROR    "[error]  "
# define LOG_LEVEL_FATAL    "[fatal]  "

# define LOG(level, msg, ...) fprintf(stderr, level msg, __VA_ARGS__)

#endif /* _LOG_H_ */