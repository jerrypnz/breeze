#include "http.h"
#include "common.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

static void _connection_close_handler(iostream_t *stream);
static void _on_http_header_data(iostream_t *stream, void *data, size_t len);
static void _set_tcp_nodelay(int fd);

connection_t* connection_accept(server_t *server, int listen_fd) {
    connection_t *conn;
    iostream_t   *stream;
    socklen_t    addr_len;
    int          conn_fd;
    struct sockaddr_in remote_addr;

        // -------- Accepting connection ----------------------------
    addr_len = sizeof(struct sockaddr_in);
    conn_fd = accept(listen_fd, (struct sockaddr*) &remote_addr, &addr_len);
    if (conn_fd == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            error("Error accepting new connection");
        return NULL;
    }

    conn = (connection_t*) calloc(1, sizeof(connection_t));
    if (conn == NULL) {
        goto error;
    }
    bzero(conn, sizeof(connection_t));

    if (set_nonblocking(conn_fd) < 0) {
        error("Error configuring Non-blocking");
        goto error;
    }

    _set_tcp_nodelay(conn_fd);
    stream = iostream_create(server->ioloop, conn_fd, 10240, 40960, conn);
    if (stream == NULL) {
        goto error;
    }
    
    iostream_set_close_handler(stream, _connection_close_handler);

    conn->server = server;
    conn->stream = stream;
    inet_ntop(AF_INET, &remote_addr.sin_addr, conn->remote_ip, 20);
    conn->remote_port = remote_addr.sin_port;
    conn->state = CONN_ACTIVE;
    conn->request  = request_create(conn);
    conn->response = response_create(conn);
    conn->context = context_create();
    
    return conn;

    error:
    close(conn_fd);
    if (conn != NULL)
        free(conn);
    return NULL;
}

int connection_close(connection_t *conn) {
    return iostream_close(conn->stream);
}

int connection_destroy(connection_t *conn) {
    request_destroy(conn->request);
    response_destroy(conn->response);
    context_destroy(conn->context);
    free(conn);
    return 0;
}

int connection_run(connection_t *conn) {
    iostream_read_until(conn->stream, "\r\n\r\n", _on_http_header_data);
    return 0;
}

static void _on_http_header_data(iostream_t *stream, void *data, size_t len) {
    connection_t   *conn;
    request_t      *req;
    response_t     *resp;
    size_t         consumed;

    conn = (connection_t*) stream->user_data;
    req = conn->request;
    resp = conn->response;

    if (req == NULL || resp == NULL) {
        connection_close(conn);
        return;
    }

    if (request_parse_headers(req, (char*)data, len, &consumed)
        != STATUS_COMPLETE) {
        connection_close(conn);
        return;
    }

    // URL Decode
    url_decode(req->path, req->path);
    if (req->query_str != NULL) {
        url_decode(req->query_str, req->query_str);   
    }

    // Handle HTTP keep-alive
    if (req->version < HTTP_VERSION_1_1 && req->connection != CONN_KEEP_ALIVE) {
        // For HTTP/1.0, default behaviour is not keep-alive unless
        // otherwise specified in request's Connection header.
        resp->connection = CONN_CLOSE;
    } else if (req->version == HTTP_VERSION_1_1 && req->connection != CONN_CLOSE) {
        // For HTTP/1.1, default behaviour is enabling keep-alive
        // unless specified explicitly.
        resp->connection = CONN_KEEP_ALIVE;
    }

    // TODO Handle Unknown HTTP version
    resp->version = req->version;
    // Reset handler configuration
    conn->context->conf = conn->server->handler_conf;
    connection_run_handler(conn, conn->server->handler);
}

void connection_run_handler(connection_t *conn, handler_func handler) {
    request_t   *req;
    response_t  *resp;
    handler_ctx_t *ctx;
    int          res;

    req = conn->request;
    resp = conn->response;
    ctx = conn->context;

    res = handler(req, resp, ctx);

    if (res == HANDLER_DONE) {
        resp->_done = 1;
    }
}

static void _connection_close_handler(iostream_t *stream) {
    connection_t  *conn;
    conn = (connection_t*) stream->user_data;
    connection_destroy(conn);
}

static void _set_tcp_nodelay(int fd) {
    int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable));
}
