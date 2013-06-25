#ifndef __IOSTREAM_H

#define __IOSTREAM_H

#include <stddef.h>
#include "ioloop.h"

struct _iostream;

typedef struct _iostream iostream_t;

typedef void (*read_handler)(iostream_t *stream, void *data, size_t len);
typedef void (*write_handler)(iostream_t *stream);
typedef void (*error_handler)(iostream_t *stream, unsigned int events);
typedef void (*close_handler)(iostream_t *stream);

iostream_t  *iostream_create(ioloop_t *loop, int sockfd, size_t read_buf_size, size_t write_buf_size);

int     iostream_close(iostream_t *stream);
int     iostream_destroy(iostream_t *stream);
int     iostream_read_bytes(iostream_t *stream, size_t sz, read_handler callback, read_handler stream_callback);
int     iostream_read_until(iostream_t *stream, char *delimiter, read_handler callback);
int     iostream_write(iostream_t *stream, void *data, size_t len, write_handler callback);
int     iostream_sendfile(iostream_t *stream, int in_fd, size_t len, write_handler callback);
int     iostream_set_error_handler(iostream_t *stream, error_handler callback);
int     iostream_set_close_handler(iostream_t *stream, close_handler callback);

#endif /* end of include guard: __IOSTREAM_H */
