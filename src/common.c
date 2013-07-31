#include "common.h"
#include <ctype.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>

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

const char* HTTP_DATE_FMT = "%a, %d %b %Y %H:%M:%S %Z";

void format_http_date(const time_t* time, char *dst, size_t len) {
    strftime(dst, len, HTTP_DATE_FMT, gmtime(time));
}

int current_http_date(char *dst, size_t len) {
    struct timeval tv;
    int res;
    res = gettimeofday(&tv, NULL);
    if (res < 0) {
        perror("Error getting time of day");
        return -1;
    }
    format_http_date(&tv.tv_sec, dst, len);
    return 0;
}
