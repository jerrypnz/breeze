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

#define DECLARE_CONF_VARIABLES()                \
    int i;                                      \
    char *name;                                 \
    json_value *val;

#define BEGIN_CONF_HANDLE(json)                   \
    for (i = 0; i < (json)->u.object.length; i++) {       \
    name = (json)->u.object.values[i].name;               \
    val = (json)->u.object.values[i].value;               \
    if (1 == 2) {}

#define ON_CONF_OPTION(expected_name, expected_type)      \
    else if (strcmp(name, (expected_name)) == 0 && val->type == (expected_type))

#define ON_STRING_CONF(expected_name, var)      \
    ON_CONF_OPTION(expected_name, json_string) { var = val->u.string.ptr; }

#define ON_INTEGER_CONF(expected_name, var)      \
    ON_CONF_OPTION(expected_name, json_integer) { var = val->u.integer; }

#define ON_BOOLEAN_CONF(expected_name, var)      \
    ON_CONF_OPTION(expected_name, json_boolean) { var = val->u.boolean; }

#define END_CONF_HANDLE() else {fprintf(stderr, "[WARN]Unknown config option %s with type %d\n", name, val->type); } }

inline void strlowercase(const char *src, char *dst, size_t n);

void format_http_date(const time_t *time, char *dst, size_t len);

int  parse_http_date(const char* str, time_t *time);

int  current_http_date(char *dst, size_t len);

int  path_starts_with(const char *prefix, const char *path);

char* url_decode(char *dst, const char *src);

#endif /* end of include guard: __COMMON_H */
