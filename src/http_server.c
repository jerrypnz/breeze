#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#define MAX_CONNECTIONS 1024
#define MAX_BACKLOG 10


static int _server_init(server_t *server);
static int _server_run(server_t *server);
static void _server_connection_handler(ioloop_t *loop,
                                       int listen_fd,
                                       unsigned int events,
                                       void *args);

server_t* server_create(unsigned short port, char *confile) {
    server_t *server;
    ioloop_t *ioloop;
    server = (server_t*) calloc(1, sizeof(server_t));
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
    ioloop_destroy(server->ioloop);
    free(server);
    return 0;
}

int server_start(server_t *server) {
    if (_server_init(server) < 0) {
        fprintf(stderr, "Error initializing server\n");
        return -1;
    }
    printf("Start running server on %d\n", server->port);
    server->state = SERVER_RUNNING;
    return ioloop_start(server->ioloop);
}

int server_stop(server_t *server) {
    printf("Stopping server\n");
    if (ioloop_stop(server->ioloop) < 0) {
        fprintf(stderr, "Error stopping ioloop\n");
        return -1;
    }
    server->state = SERVER_STOPPED;
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
    if (ioloop_add_handler(server->ioloop,
                           listen_fd,
                           EPOLLIN,
                           _server_connection_handler,
                           server) < 0) {
        fprintf(stderr, "Error add connection handler\n");
        return -1;
    }
    return 0;
}

static void _server_connection_handler(ioloop_t *loop,
                                       int listen_fd,
                                       unsigned int events,
                                       void *args)
{
    connection_t *conn;
    server_t     *server = (server_t*) args;

    conn = connection_accept(server, listen_fd);
    if (conn == NULL) {
        fprintf(stderr, "Error creating connection\n");
        return;
    }
    
    connection_run(conn);
}


