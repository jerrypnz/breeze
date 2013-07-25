#ifndef __BUFFER_H

#define __BUFFER_H

#include <unistd.h>

struct _buffer;

typedef struct _buffer buffer_t;

typedef void (*consumer_func)(void *data, size_t len, void *args);

buffer_t    *buffer_create(size_t size);
int          buffer_destroy(buffer_t *buf);
int          buffer_is_full(buffer_t *buf);
int          buffer_is_empty(buffer_t *buf);
int          buffer_put(buffer_t *buf, void *data, size_t len);
size_t       buffer_get(buffer_t *buf, size_t len, void *target, size_t capacity);
ssize_t      buffer_fill(buffer_t *buf, int fd);
ssize_t      buffer_flush(buffer_t *buf, int fd);
size_t       buffer_consume(buffer_t *buf, size_t len, consumer_func cb, void *args);
int          buffer_locate(buffer_t *buf, char *delimiter);

#endif /* end of include guard: __BUFFER_H */
