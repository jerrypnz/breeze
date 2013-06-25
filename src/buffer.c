#include "buffer.h"
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef unsigned char byte_t;

struct _buffer {
    byte_t     *data;
    size_t      size;
    size_t      capacity;
    int         head;
    int         tail;
};

__inline__ static void _do_write(buffer_t *buf, byte_t *data, size_t len);
__inline__ static void _do_read_to(buffer_t *buf, byte_t *target, size_t len);
__inline__ static void _do_consume(buffer_t *buf, size_t len, consumer_func func, void *args);
__inline__ static ssize_t _do_read_to_fd(buffer_t *buf, int to_fd, size_t len);
__inline__ static ssize_t _do_write_from_fd(buffer_t *buf, int from_fd, size_t len);


buffer_t *buffer_create(size_t size) {
    buffer_t    *buf;
    void        *mem;
    size_t      mem_sz;

    mem_sz = size + sizeof(buffer_t);
    mem = malloc(mem_sz);
    if (mem == NULL) {
        perror("Error allocating buffer memory");
        return NULL;
    }
    buf = (buffer_t*) mem;
    buf->capacity = size;
    buf->size = 0;
    buf->head = 0;
    buf->tail = 0;
    buf->data = (byte_t*) mem + sizeof(buffer_t);

    return buf;
}


int buffer_destroy(buffer_t *buf) {
    if (buf == NULL)
        return -1;
    free(buf);
    return 0;
}


int buffer_is_full(buffer_t *buf) {
    return buf->size == buf->capacity;
}


int buffer_is_empty(buffer_t *buf) {
    return buf->size == 0;
}


int buffer_write(buffer_t *buf, void *data, size_t len) {
    size_t  cap;
    size_t  write_len;
    byte_t  *_data = (byte_t*) data;

    cap = buf->capacity - buf->size;
    if (len > cap) {
        return -1;
    }

    write_len = MIN(len, buf->capacity - buf->tail);
    _do_write(buf, _data, write_len);

    _data += write_len;
    write_len = len - write_len;
    if (write_len > 0) {
        _do_write(buf, _data, write_len);
    }
    return 0;
}


ssize_t buffer_write_from_fd(buffer_t *buf, int fd, size_t len) {
    ssize_t  n, iovcnt;
    struct iovec iov[2];
    
    iov[0].iov_base = buf->data + buf->tail;
    if (buf->head > buf->tail) {
        iov[0].iov_len = buf->head - buf->tail;
        iovcnt = 1;
    } else {
        iov[0].iov_len = buf->capacity - buf->tail;
        iov[1].iov_base = buf->data;
        iov[1].iov_len = buf->head;
        iovcnt = 2;
    }

    n = readv(fd, iov, iovcnt);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        } else {
            return -1;
        }
    } else if (n == 0) {
        return -2;
    }
    buf->size += n;
    buf->tail = (buf->tail + n) % buf->capacity;   
    return n;
}


size_t buffer_read_to(buffer_t *buf, size_t len, void *target, size_t capacity) {
    size_t      read_len, total = 0;
    byte_t      *_target = (byte_t*) target;

    len = MIN(buf->size, len);
    read_len = MIN(len, buf->capacity - buf->head);
    read_len = MIN(read_len, capacity);
    _do_read_to(buf, _target, read_len);

    total += read_len;
    _target += read_len;
    capacity -= read_len;
    read_len = MIN(capacity, len - read_len);

    if (read_len > 0) {
        _do_read_to(buf, _target, read_len);
        total += read_len;
    }

    return total;
}


size_t buffer_consume(buffer_t *buf, size_t len, consumer_func cb, void *args) {
    size_t      read_len, total = 0;

    len = MIN(buf->size, len);
    read_len = MIN(len, buf->capacity - buf->head);
    _do_consume(buf, read_len, cb, args);

    total += read_len;
    read_len = len - read_len;

    if (read_len > 0) {
        _do_consume(buf, read_len, cb, args);
        total += read_len;
    }

    return total;
}


ssize_t buffer_read_to_fd(buffer_t *buf, size_t len, int to_fd) {
    size_t      read_len;
    ssize_t     total, n;

    len = MIN(buf->size, len);
    total = 0;
    read_len = MIN(len, buf->capacity - buf->head);
    n = _do_read_to_fd(buf, to_fd, read_len);

    if (n < 0) {
        return -1;
    }

    // If the first part is not written completely, we
    // just return and don't try the next part.
    if (n < read_len) {
        return n;
    }

    total += read_len;
    read_len = len - read_len;

    if (read_len > 0) {
        n = _do_read_to_fd(buf, to_fd, read_len);
        if (n < 0) {
            return -1;
        }
        total += n;
    }

    return total;
}


#define MAX_DELIM_LEN 20

int buffer_locate(buffer_t *buf, char *delimiter) {
    char    tmp[MAX_DELIM_LEN];
    char    last_char;
    char    *sub;
    int     idx = -1;
    size_t  delim_len;

    if (buf->size <= 0 || *delimiter == '\0') {
        return -1;
    }

    if (buf->head < buf->tail) {
        last_char = buf->data[buf->tail];
        buf->data[buf->tail] = '\0';
        sub = strstr((char*)buf->data + buf->head, delimiter);
        buf->data[buf->tail] = last_char;
        if (sub != NULL) {
            idx = (sub - (char*)buf->data) - buf->head;
            goto finish;
        }
    } else {
        last_char = buf->data[buf->capacity - 1];
        buf->data[buf->capacity - 1] = '\0';
        sub = strstr((char*)buf->data + buf->head, delimiter);
        buf->data[buf->capacity - 1] = last_char;
        if (sub != NULL) {
            idx = (sub - (char*)buf->data) - buf->head;
            goto finish;
        }

        delim_len = strlen(delimiter);
        memcpy(tmp, buf->data + buf->capacity - delim_len, delim_len);
        memcpy(tmp + delim_len, buf->data, delim_len);
        tmp[delim_len * 2 + 1] = '\0';
        sub = strstr(tmp, delimiter);
        if (sub != NULL) {
            idx = buf->capacity - delim_len - buf->head + (sub - tmp);
            goto finish;
        }

        last_char = buf->data[buf->tail];
        buf->data[buf->tail] = '\0';
        sub = strstr((char*)buf->data, delimiter);
        buf->data[buf->tail] = last_char;
        if (sub != NULL) {
            idx = (sub - (char*)buf->data) + (buf->capacity - buf->head);
            goto finish;
        }
    }

finish:
    return idx;
}

__inline__ static void _do_write(buffer_t *buf, byte_t *data, size_t len) {
    memcpy(buf->data + buf->tail, data, len);
    buf->size += len;
    buf->tail = (buf->tail + len) % buf->capacity;
}

__inline__ static void _do_read_to(buffer_t *buf, byte_t *target, size_t len) {
    memcpy(target, buf->data + buf->head, len);
    buf->size -= len;
    buf->head = (buf->head + len) % buf->capacity;
}

__inline__ static void _do_consume(buffer_t *buf, size_t len, consumer_func func, void *args) {
    void *data = buf->data + buf->head;
    buf->size -= len;
    buf->head = (buf->head + len) % buf->capacity;
    func(data, len, args);
}

__inline__ static ssize_t _do_read_to_fd(buffer_t *buf, int to_fd, size_t len) {
    ssize_t     n, total = 0;
    while (len > 0) {
        n = write(to_fd, buf->data + buf->head, len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                return -1;
            }
        } else if (n == 0) {
            break;
        }
        len -= n;
        total += n;
        buf->size -= n;
        buf->head = (buf->head + n) % buf->capacity;
    }
    return total;
}


__inline__ static ssize_t _do_write_from_fd(buffer_t *buf, int from_fd, size_t len) {
    ssize_t     n, total = 0;
    while(total < len) {
        n = read(from_fd, buf->data + buf->tail, len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                return -1;
            }
        }
        if (n == 0) {
            return -2;
        }
        total += n;
        buf->size += n;
        buf->tail = (buf->tail + n) % buf->capacity;
    }
    return total;
}
