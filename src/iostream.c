#include "iostream.h"
#include "ioloop.h"
#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>

enum READ_OP_TYPES {
    READ_BYTES = 1,
    READ_UNTIL = 2
};

enum STREAM_STATE {
    NORMAL = 1,
    CLOSED = 2
};

#define is_reading(stream) ((stream)->read_callback != NULL)
#define is_writing(stream) ((stream)->write_callback != NULL)
#define is_closed(stream)  ((stream)->state == CLOSED)

#define check_reading(stream)  \
    if (is_reading(stream)) {  \
        return -1;             \
    }

#define check_writing(stream)  \
    if (is_writing(stream)) {  \
        return -1;             \
    }

static void _handle_io_events(ioloop_t *loop, int fd, unsigned int events, void *args);
static void _handle_error(iostream_t *stream, unsigned int events);
static int  _handle_read(iostream_t *stream);
static int  _handle_write(iostream_t *stream);
static int _add_event(iostream_t *stream, unsigned int events);

static ssize_t _read_from_socket(iostream_t *stream);
static int     _read_from_buffer(iostream_t *stream);
static ssize_t _write_to_buffer(iostream_t *stream, void *data, size_t len);
static int     _write_to_socket(iostream_t *stream);
static ssize_t _write_to_socket_direct(iostream_t *stream, void *data, size_t len);

static void _finish_stream_callback(ioloop_t *loop, void *args);
static void _finish_read_callback(ioloop_t *loop, void *args);
static void _finish_write_callback(ioloop_t *loop, void *args);
static void _close_callback(ioloop_t *loop, void *args);
static void _destroy_callback(ioloop_t *loop, void *args);

static void _stream_consumer_func(void *data, size_t len, void *args);

iostream_t *iostream_create(ioloop_t *loop,
                            int sockfd,
                            size_t read_buf_capacity,
                            size_t write_buf_capacity,
                            void *user_data) {
    iostream_t  *stream;
    buffer_t    *in_buf, *out_buf;

    stream = (iostream_t*) calloc(1, sizeof(iostream_t));
    if (stream == NULL) {
        perror("Error allocating memory for IO stream");
        goto error;
    }
    bzero(stream, sizeof(iostream_t));

    in_buf = buffer_create(read_buf_capacity);
    if (in_buf == NULL ) {
        perror("Error creating read buffer");
        goto error;
    }
    out_buf = buffer_create(write_buf_capacity);
    if (out_buf == NULL) {
        perror("Error creating write buffer");
        goto error;
    }

    stream->events = EPOLLERR;
    stream->write_buf = out_buf;
    stream->write_buf_cap = write_buf_capacity;
    stream->read_buf = in_buf;
    stream->read_buf_cap = read_buf_capacity;
    stream->fd = sockfd;
    stream->state = NORMAL;
    stream->ioloop = loop;
    stream->read_callback = NULL;
    stream->write_callback = NULL;
    stream->close_callback = NULL;
    stream->error_callback = NULL;
    stream->sendfile_fd = -1;
    stream->user_data = user_data;

    if (ioloop_add_handler(stream->ioloop,
                           stream->fd,
                           stream->events,
                           _handle_io_events,
                           stream) < 0) {
        perror("Error add EPOLLERR event");
        goto error;
    }

    return stream;

error:
    buffer_destroy(in_buf);
    buffer_destroy(out_buf);
    free(stream);
    return NULL;
}

int iostream_close(iostream_t *stream) {
    if (is_closed(stream)) {
        return -1;
    }
    stream->state = CLOSED;
    // Defer the close action to next loop, because there may be
    // pending read/write operations.
    ioloop_add_callback(stream->ioloop, _close_callback, stream);
    return 0;
}

static void _close_callback(ioloop_t *loop, void *args) {
    iostream_t  *stream = (iostream_t*) args;
    ioloop_remove_handler(stream->ioloop, stream->fd);
    stream->close_callback(stream);
    close(stream->fd);
    // Defer the destroy action to next loop, in case there are
    // pending callbacks of this stream.
    ioloop_add_callback(stream->ioloop, _destroy_callback, stream);
}

static void _destroy_callback(ioloop_t *loop, void *args) {
    iostream_t *stream = (iostream_t*) args;
    printf("IO stream(fd:%d) destroyed.\n", stream->fd);
    iostream_destroy(stream);
}
    
int iostream_destroy(iostream_t *stream) {
    buffer_destroy(stream->read_buf);
    buffer_destroy(stream->write_buf);
    free(stream);
    return 0;
}

int iostream_read_bytes(iostream_t *stream,
                        size_t sz,
                        read_handler callback,
                        read_handler stream_callback) {
    check_reading(stream);
    if (sz == 0) {
        return -1;
    }
    stream->read_callback = callback;
    stream->stream_callback = stream_callback;
    stream->read_bytes = sz;
    stream->read_type = READ_BYTES;
    for (;;) {
        if (_read_from_buffer(stream)) {
            return 0;
        }
        if (is_closed(stream)) {
            return -1;
        }
        if (_read_from_socket(stream) == 0) {
            break;
        }
    }
    _add_event(stream, EPOLLIN);
    return 0;
}

int iostream_read_until(iostream_t *stream, char *delimiter, read_handler callback) {
    check_reading(stream);
    assert(*delimiter != '\0');
    stream->read_callback = callback;
    stream->stream_callback = NULL;
    stream->read_delimiter = delimiter;
    stream->read_type = READ_UNTIL;
    for (;;) {
        if (_read_from_buffer(stream)) {
            return 0;
        }
        if (is_closed(stream)) {
            return -1;
        }
        if (_read_from_socket(stream) == 0) {
            break;
        }
    }
    _add_event(stream, EPOLLIN);
    return 0;
}

int iostream_write(iostream_t *stream, void *data, size_t len, write_handler callback) {
    ssize_t     n;
    // Allow appending data to existing writing action
    if (is_writing(stream) && callback != stream->write_callback) {
        return -1;
    }

    if (len == 0) {
        return -1;
    }

    stream->write_callback = callback;
    n = _write_to_socket_direct(stream, data, len);
    if (n < 0) {
        return -1;
    } else if (n == len) {
        // Lucky! All the data are written directly, skipping the buffer completely.
        return 0;
    }

    if (_write_to_buffer(stream, (char*)data + n, len - n) < 0) {
        return -1;
    }

    // Try to write to the socket
    if (_write_to_socket(stream) > 0) {
        return 0;
    }
    _add_event(stream, EPOLLOUT);
    return 0;
}

int iostream_sendfile(iostream_t *stream, int in_fd, size_t len, write_handler callback) {
    // TODO Implement sendfile
    return 0;
}

int iostream_set_error_handler(iostream_t *stream, error_handler callback) {
    stream->error_callback = callback;
    return 0;
}

int iostream_set_close_handler(iostream_t *stream, close_handler callback) {
    stream->close_callback = callback;
    return 0;
}

static void _handle_error(iostream_t *stream, unsigned int events) {
    error_handler err_cb = stream->error_callback;
    if (err_cb != NULL)
        stream->error_callback(stream, events);
    iostream_close(stream);
}

static void _handle_io_events(ioloop_t *loop,
                              int fd,
                              unsigned int events,
                              void *args) {
    iostream_t      *stream = (iostream_t*) args;

    if (events & EPOLLIN) {
        printf("Handling read on socket fd %d\n", stream->fd);
        if (_handle_read(stream)) {
            stream->events &= ~EPOLLIN;
        }
    }
    if (events & EPOLLOUT) {
        printf("Handling write on socket fd %d\n", stream->fd);
        if (_handle_write(stream)) {
            stream->events &= ~EPOLLOUT;
        }
    }
    if (events & EPOLLERR) {
        printf("Handling error on socket fd %d\n", stream->fd);
        _handle_error(stream, events);
        return;
    }
    if (events & EPOLLHUP) {
        printf("Handling hangup on socket fd %d\n", stream->fd);
        iostream_close(stream);
        return;
    }

    if (is_closed(stream)) {
        fprintf(stderr, "Stream closed");
        return;
    }

    printf("Updating epoll events for socket fd %d, event %d\n",
           stream->fd, stream->events);
    ioloop_update_handler(stream->ioloop, stream->fd, stream->events);    
}

static int _handle_read(iostream_t *stream) {
    if (!is_reading(stream)) {
        return 0;
    }
    _read_from_socket(stream);
    return _read_from_buffer(stream);
}

static int _handle_write(iostream_t *stream) {
    return _write_to_socket(stream);
}

static int _add_event(iostream_t *stream, unsigned int event) {
    if ((stream->events & event) == 0) {
        stream->events |= event;
        return ioloop_update_handler(stream->ioloop, stream->fd, stream->events);
    }
    return -1;
}

#define READ_SIZE 1024

static ssize_t _read_from_socket(iostream_t *stream) {
    ssize_t  n;
    n = buffer_write_from_fd(stream->read_buf, stream->fd, READ_SIZE);
    if (n < 0) {
        iostream_close(stream);
        return -1;
    }
    stream->read_buf_size += n;
    return n;
}


#define LOCAL_BUFSIZE 4096

static int _read_from_buffer(iostream_t *stream) {
    int     res = 0, idx;

    switch(stream->read_type) {
        case READ_BYTES:
            if (stream->stream_callback != NULL) {
                // Streaming mode, offer data
                if (stream->read_bytes <= 0) {
                    res = 1;
                } else {
                    ioloop_add_callback(stream->ioloop, _finish_stream_callback, stream);
                }
            } else if (stream->read_buf_size >= stream->read_bytes
                       || buffer_is_full(stream->read_buf)
                       || (stream->state == CLOSED && stream->read_buf_size > 0)) {
                ioloop_add_callback(stream->ioloop, _finish_read_callback, stream);
                res = 1;
            }
            break;

        case READ_UNTIL:
            idx = buffer_locate(stream->read_buf, stream->read_delimiter);
            if (idx > 0
                || buffer_is_full(stream->read_buf)
                || (stream->state == CLOSED && stream->read_buf_size > 0)) {
                stream->read_bytes = idx > 0
                    ? (idx + strlen(stream->read_delimiter))
                    : stream->read_buf_size;
                ioloop_add_callback(stream->ioloop, _finish_read_callback, stream);
                res = 1;
            }
            break;
    }

    return res;
}


static void _finish_stream_callback(ioloop_t *loop, void *args) {
    iostream_t  *stream = (iostream_t*) args;
    read_handler    callback = stream->read_callback;

    buffer_consume(stream->read_buf, stream->read_bytes, _stream_consumer_func, stream);
    if (stream->read_bytes <= 0) {
        stream->read_callback = NULL;
        stream->read_bytes = 0;
        // When streaming ends, call the read_callback with NULL to indicate the finish.
        callback(stream, NULL, 0);
    }
}

static void _finish_read_callback(ioloop_t *loop, void *args) {
    char            local_buf[LOCAL_BUFSIZE];
    iostream_t      *stream = (iostream_t*) args;
    read_handler    callback = stream->read_callback;
    size_t          n;

    // Normal mode, call read callback
    n = buffer_read_to(stream->read_buf, stream->read_bytes, local_buf, LOCAL_BUFSIZE);
    // assert(n == stream->read_bytes);
    callback = stream->read_callback;
    stream->read_callback = NULL;
    stream->read_bytes = 0;
    stream->read_buf_size -= n;
    callback(stream, local_buf, n);
}

static void _finish_write_callback(ioloop_t *loop, void *args) {
    iostream_t      *stream = (iostream_t*) args;
    write_handler   callback = stream->write_callback;

    stream->write_callback = NULL;
    if (callback != NULL) {
        callback(stream);
    }
}

static void _stream_consumer_func(void *data, size_t len, void *args) {
    iostream_t  *stream = (iostream_t*) args;
    stream->read_bytes -= len;
    stream->read_buf_size -= len;
    stream->stream_callback(stream, data, len);
}

static ssize_t _write_to_buffer(iostream_t *stream, void *data, size_t len) {
    if (len > stream->write_buf_cap - stream->write_buf_size) {
        return -1;
    }
    if (buffer_write(stream->write_buf, data, len) < 0) {
        return -1;
    }
    stream->write_buf_size += len;
    return 0;
}

#define WRITE_CHUNK_SIZE 1024

static int _write_to_socket(iostream_t *stream) {
    ssize_t         n;

    for(;;) {
        n = buffer_read_to_fd(stream->write_buf, WRITE_CHUNK_SIZE, stream->fd);
        if (n < 0) {
            iostream_close(stream);
            return -1;
        }
        stream->write_buf_size -= n;
        if (n < WRITE_CHUNK_SIZE) {
            break;
        }
    }
    if (stream->write_buf_size <= 0) {
        ioloop_add_callback(stream->ioloop, _finish_write_callback, stream);
        return 1;
    } else {
        return 0;
    }
}

static ssize_t _write_to_socket_direct(iostream_t *stream, void *data, size_t len) {
    ssize_t         n;

    if (stream->write_buf_size > 0) {
        // If there is data in the buffer, we could not write to socket directly.
        return 0;
    }

    n = write(stream->fd, data, len);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        } else {
            iostream_close(stream);
            return -1;
        }
    }

    if (n == len) {
        // If we could write all the data once, call the callback function now.
        ioloop_add_callback(stream->ioloop, _finish_write_callback, stream);
    }

    return n;
}

