#ifndef __COMMON_H
#define __COMMON_H

#include <stddef.h>

enum _return_status {
    STATUS_ERROR = -1,
    STATUS_COMPLETE = 0,
    STATUS_INCOMPLETE = 1
};

#define MIN(a, b) ((a) < (b) ? (a) : (b))

inline void strlowercase(const char *src, char *dst, size_t n);

#endif /* end of include guard: __COMMON_H */
