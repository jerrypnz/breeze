#ifndef __IOLOOP_H

#define __IOLOOP_H


#define MAX_TIMEOUTS    100


struct _ioloop;

typedef struct _ioloop ioloop_t;

typedef void (*io_handler)(ioloop_t *loop, int fd, unsigned int events, void *args);
typedef void (*callback_handler)(ioloop_t *loop, void *args);

ioloop_t    *ioloop_create(unsigned int maxfds);
int          ioloop_destroy(ioloop_t *loop);
int          ioloop_start(ioloop_t *loop);
int          ioloop_stop(ioloop_t *loop);
int          ioloop_add_handler(ioloop_t *loop, int fd, unsigned int events, io_handler handler, void *args);
int          ioloop_update_handler(ioloop_t *loop, int fd, unsigned int events);
io_handler   ioloop_remove_handler(ioloop_t *loop, int fd);
int          ioloop_add_callback(ioloop_t *loop, callback_handler handler, void *args);

int     set_nonblocking(int sockfd);

#endif /* end of include guard: __IOLOOP_H */
