#ifndef __LOG_H_
#define __LOG_H_

enum _log_level {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR
};

int configure_log(int level, const char* file, int use_console);

void logging(int lvl, const char *file, const int line, const char *fmt, ...);

#define debug(fmt, ...) logging(DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define info(fmt, ...)  logging(INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define warn(fmt, ...)  logging(WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define error(fmt, ...) logging(ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif /* __LOG_H_ */
