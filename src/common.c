#include "common.h"
#include <ctype.h>

inline void strlowercase(const char *src, char *dst, size_t n) {
    int i;
    const char *p;
    for (i = 0, p = src;
         i < n && *p != '\0';
         i++, p++) {
        dst[i] = tolower(*p);
    }
    dst[i] = '\0';
}
