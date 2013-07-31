#ifndef __COMMON_H
#define __COMMON_H

#define _XOPEN_SOURCE
#define _GNU_SOURCE
#define _BSD_SOURCE

#include <stddef.h>
#include <time.h>

enum _return_status {
    STATUS_ERROR = -1,
    STATUS_COMPLETE = 0,
    STATUS_INCOMPLETE = 1
};

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define _BREEZE_NAME "breeze/0.1.0"

inline void strlowercase(const char *src, char *dst, size_t n);

void format_http_date(const time_t *time, char *dst, size_t len);

int  parse_http_date(const char* str, time_t *time);

int current_http_date(char *dst, size_t len);

#endif /* end of include guard: __COMMON_H */
