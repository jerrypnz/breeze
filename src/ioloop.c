#include "ioloop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#define MAX_CALLBACKS 10000

struct _callback {
    callback_handler    callback;
    void          *args;
};

struct _io_callback {
    io_handler      callback;
    void      *args;
};

struct _callback_chain {
    struct _callback    callbacks[MAX_CALLBACKS];
    int                 callback_num;
};

struct _ioloop {
    int                 epoll_fd;
    int                 state;
    struct _io_callback *handlers;
    struct _callback_chain  callback_chains[2];
    int                     callback_chain_idx;
};

enum IOLOOP_STATES {
    INITIALIZED = 0,
    RUNNING = 1,
    STOPPED = 2
};


ioloop_t *ioloop_create(unsigned int maxfds) {
    ioloop_t                 *loop = NULL;
    int                      epoll_fd;
    struct _io_callback      *handlers = NULL;
    
    loop = (ioloop_t*) calloc (1, sizeof(ioloop_t));
    if (loop == NULL) {
        perror("Could not allocate memory for IO loop");
        return NULL;
    }
    bzero(loop, sizeof(ioloop_t));


    handlers = (struct _io_callback*) calloc(maxfds, sizeof(struct _io_callback));
    if (handlers == NULL) {
        perror("Could not allocate memory for IO handlers");
        return NULL;
    }

    bzero(handlers, maxfds * sizeof(struct _io_callback));


    epoll_fd = epoll_create(maxfds);
    if (epoll_fd == -1) {
        perror("Error initializing epoll");
        return NULL;
    }

    loop->handlers = handlers;
    loop->epoll_fd = epoll_fd;
    loop->state = INITIALIZED;
    loop->callback_chains[0].callback_num = 0;
    loop->callback_chains[1].callback_num = 0;
    loop->callback_chain_idx = 0;
    return loop;
}


int ioloop_destroy(ioloop_t *loop) {
    free(loop->handlers);
    free(loop);
    return 0;
}

int ioloop_add_handler(ioloop_t *loop,
                       int fd,
                       unsigned int events,
                       io_handler handler,
                       void *args) {
    struct epoll_event     ev;

    if (handler == NULL) {
        fprintf(stderr, "Handler should not be NULL!");
        return -1;
    }

    ev.data.fd = fd;
    ev.events = events | EPOLLET;
    if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("Error adding fd to epoll");
        return -1;
    }

    loop->handlers[fd].callback = handler;
    loop->handlers[fd].args = args;
    return 0;
}

int ioloop_update_handler(ioloop_t *loop, int fd, unsigned int events) {
    struct epoll_event     ev;

    ev.data.fd = fd;
    ev.events = events | EPOLLET;
    if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1) {
        perror("Error modifying epoll events");
        return -1;
    }

    return 0;
}


io_handler  ioloop_remove_handler(ioloop_t *loop, int fd) {
    int         res;
    io_handler  handler;
    printf("Removing handler for fd %d\n", fd);
    handler = loop->handlers[fd].callback;
    loop->handlers[fd].callback = NULL;
    loop->handlers[fd].args = NULL;
    res = epoll_ctl(loop->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    if (res < 0) {
        perror("Error removing fd from epoll");
    }
    return handler;
}


#define MAX_EVENTS    1024
#define EPOLL_DEFAULT_TIMEOUT 100

int ioloop_start(ioloop_t *loop) {
    struct epoll_event  events[MAX_EVENTS];
    struct _callback_chain  *current_chain;
    int                 epoll_fd, nfds, i, fd, epoll_timeout;
    io_handler          handler;
    void          *args;

    if (loop->state != INITIALIZED) {
        fprintf(stderr, "Could not restart an IO loop");
        return -1;
    }
    epoll_fd = loop->epoll_fd;
    loop->state = RUNNING;
    while (loop->state == RUNNING) {
        // Handle callbacks
        current_chain = loop->callback_chains + loop->callback_chain_idx;
        if (current_chain->callback_num > 0) {
            loop->callback_chain_idx = (loop->callback_chain_idx + 1) % 2;
            for (i = 0; i < current_chain->callback_num; i++) {
                current_chain->callbacks[i].callback(loop, current_chain->callbacks[i].args);
            }
            current_chain->callback_num = 0;
        }

        // Wait for events
        epoll_timeout = EPOLL_DEFAULT_TIMEOUT;
        if (loop->callback_chains[loop->callback_chain_idx].callback_num > 0) {
            // There are callbacks that needs running, so we do not wait in epoll_wait
            epoll_timeout = 0;
        }
        nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, epoll_timeout);
        if (nfds == -1) {
            perror("epoll_wait");
            continue;
        }

        // Handle events
        for (i = 0; i < nfds; i++) {
            fd = events[i].data.fd;
            handler = loop->handlers[fd].callback;
            args = loop->handlers[fd].args;
            if (handler == NULL) {
                continue;
            }
            handler(loop, fd, events[i].events, args);
        }
    }

    close(epoll_fd);
    //TODO Other cleanup work here.
    return 0;
}


int ioloop_stop(ioloop_t *loop) {
    loop->state = STOPPED;
    return 0;
}


int ioloop_add_callback(ioloop_t *loop, callback_handler handler, void *args) {
    struct _callback_chain  *current_chain = loop->callback_chains + loop->callback_chain_idx;
    int n = current_chain->callback_num;
    if (n >= MAX_CALLBACKS) {
        return -1;
    }
    current_chain->callbacks[n].callback = handler;
    current_chain->callbacks[n].args = args;
    current_chain->callback_num ++;
    return 0;
}

int set_nonblocking(int sockfd) {
    int opts;
    opts = fcntl(sockfd, F_GETFL);
    if (opts < 0) 
        return -1;
    opts |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, opts) < 0)
        return -1;
    return 0;
}
