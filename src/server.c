#include "ioloop.h"
#include "iostream.h"
#include "http.h"
#include "connection.h"

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
#include <netinet/in.h>

#define MAX_CONNECTIONS 1024

typedef enum _server_state {
    SERVER_INIT = 0,
    SERVER_RUNNING,
    SERVER_STOPPED
} server_state;

struct _server {
    unsigned short  port;
    handler_func    handler;

    server_state    state;
    int             listen_fd;
    ioloop_t        *ioloop;
};


static int _server_init(server_t *server);
static int _server_run(server_t *server);
static void _server_connection_close_handler(iostream_t *stream);
static void _on_http_header_data(iostream_t *stream, void *data, size_t len);
static int _set_nonblocking(int sockfd);

server_t* server_create(unsigned short port, char *confile) {
    server_t *server;
    ioloop_t *ioloop;
    server = (server_*) calloc(1, sizeof(server_t));
    if (server == NULL) {
        perror("Error allocating memory for server");
        return NULL;
    }

    ioloop = (ioloop_t*) ioloop_create(MAX_CONNECTIONS);
    if (ioloop == NULL) {
        fprintf(stderr, "Error creating ioloop");
        return NULL;
    }
    // TODO Parse and create site?
    server->port = port;
    server->ioloop = ioloop;
    server->state = SERVER_INIT;
}

int server_destroy(server_t *server) {
    site_destroy(server->site);
    ioloop_destroy(server->ioloop);

    free(server);
    return 0;
}

int server_start(server_t *server) {
    if (_server_init(server) < 0) {
        fprintf("Error initializing server");
        return -1;
    }
    printf("Start running server on %d", server->port);
    return ioloop_start(server->ioloop);
}

int server_stop(server_t *server) {
    printf("Stopping server");
    if (ioloop_stop(server->ioloop) < 0) {
        fprintf("Error stopping ioloop");
        return -1;
    }
    server->state = STOPPED;
    return 0;
}

static int _server_init(server_t *server) {
    int                     listen_fd;
    struct sockaddr_in      addr;

    // ---------- Create and bind listen socket fd --------------
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("Error creating socket");
        return -1;
    }

    bzero(&addr, sizeof(struct sockaddr_in));
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(server->port);

    if (bind(listen_fd,
             (struct sockaddr *)&addr,
             sizeof(struct sockaddr_in)) == -1) {
        perror("Error binding address");
        return -1;
    }

    // ------------ Start listening ------------------------------
    if (listen(listen_fd, MAX_BACKLOG) == -1) {
        perror("Error listening");
        return -1;
    }
    server->listen_fd = listen_fd;
    if (ioloop_add_handler(loop,
                           listen_fd,
                           EPOLLIN,
                           _server_connection_handler,
                           server) < 0) {
        fprintf(stderr, "Error add connection handler");
        return -1;
    }
    return 0;
}

static void _server_connection_handler(ioloop_t *loop,
                                       int listen_fd,
                                       unsigned int events,
                                       void *args)
{
    server_t     *server;
    connection_t *conn;
    iostream_t   *stream;
    socklen_t    addr_len;
    int          conn_fd;
    struct sockaddr_in  remo_addr;

    server = (server_t*) args;

    // -------- Accepting connection ----------------------------
    printf("Accepting new connection...\n");
    addr_len = sizeof(struct sockaddr_in);
    conn_fd = accept(listen_fd, (struct sockaddr*) &remo_addr, &addr_len);
    printf("Connection fd: %d...\n", conn_fd);
    if (conn_fd == -1) {
        perror("Error accepting new connection");
        return;
    }

    if (_set_nonblocking(conn_fd) < 0) {
        perror("Error configuring Non-blocking");
        return;
    }
    
    stream = iostream_create(loop, conn_fd, 1024, 1024);
    if (stream == NULL) {
        fprintf(stderr, "Error creating iostream");
        return;
    }
    
    conn = connection_create(server, stream);
    if (conn == NULL) {
        fprintf(stderr, "Error creating connection");
        return;
    }
    
    iostream_set_close_handler(stream, connection_close_handler);
    connection_run(conn);
}

static int _set_nonblocking(int sockfd) {
    int opts;
    opts = fcntl(sockfd, F_GETFL);
    if (opts < 0) 
        return -1;
    opts |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, opts) < 0)
        return -1;
    return 0;
}

static void _server_connection_handler(iostream_t *stream) {
    //TODO handle connection close
}
