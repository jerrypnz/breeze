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

int parse_http_date(const char* str, time_t *time) {
    struct tm  tm;
    if (strptime(str, HTTP_DATE_FMT, &tm) == NULL) {
        return -1;
    }
    // Assume it's GMT time since strptime does not really support %Z
    *time = timegm(&tm);
    return 0;
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

int path_starts_with(const char* prefix, const char* path) {
    int match_count;

    if (path == NULL || prefix ==  NULL)
        return 0;

    match_count = 0;

    while (*path != '\0' && *prefix != '\0' && *path == *prefix) {
        match_count++;
        path++;
        prefix++;
    }

    if (*prefix == '\0' && (*path == '\0' || *path == '/')) 
        return match_count; 
    else 
        return 0;
}
