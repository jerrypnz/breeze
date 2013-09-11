#ifndef __HTTP_H
#define __HTTP_H

#include "common.h"
#include "json.h"
#include "ioloop.h"
#include "iostream.h"
#include <search.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stddef.h>
#include <time.h>

#define REQUEST_BUFFER_SIZE     2048
#define RESPONSE_BUFFER_SIZE    2048
#define MAX_HEADER_SIZE         25
#define MAX_STATE_STACK_SIZE    256

/*
 * HTTP request/response
 */
typedef struct _request         request_t;
typedef struct _response        response_t;
typedef struct _http_header     http_header_t;
typedef struct _http_status     http_status_t;
typedef union  _ctx_state       ctx_state_t;
typedef struct _handler_ctx     handler_ctx_t;
typedef struct _state_node        ctx_node_t;

/*
 * HTTP server/connection
 */
typedef struct _server server_t;
typedef struct _connection connection_t;

enum _handler_result {
    HANDLER_DONE,
    HANDLER_UNFISHED,
};

/*
 * HTTP handler function
 */
typedef int (*handler_func)(request_t *request, response_t *response, handler_ctx_t *ctx);

request_t*   request_create(connection_t *conn);
int          request_reset(request_t *req);
int          request_destroy(request_t *request);
const char*  request_get_header(request_t *request, const char *header_name);
int          request_parse_headers(request_t *request,
                                   const char *data,
                                   const size_t data_len,
                                   size_t *consumed);

response_t*    response_create(connection_t *conn);
int            response_reset(response_t *resp);
int            response_destroy(response_t *response);
const char*    response_get_header(response_t *response, const char *header_name);
int            response_set_header(response_t *response, char *name, char *value);
int            response_set_header_printf(response_t *response, char* name,
                                          const char *fmt, ...);
char*          response_alloc(response_t *response, size_t n);
int            response_write(response_t *response,
                              char *data,
                              size_t data_len,
                              handler_func next_handler);
int            response_send_file(response_t *response,
                                  int fd,
                                  size_t offset,
                                  size_t size,
                                  handler_func next_handler);
int            response_send_status(response_t *response, http_status_t status);
int            response_send_headers(response_t *response, handler_func next_handler);

handler_ctx_t* context_create();
int            context_destroy(handler_ctx_t *ctx);
int            context_reset(handler_ctx_t *ctx);
int            context_push(handler_ctx_t *ctx, ctx_state_t stat);
ctx_state_t*   context_pop(handler_ctx_t *ctx);
ctx_state_t*   context_peek(handler_ctx_t *ctx);

connection_t*  connection_accept(server_t *server, int listen_fd); 
int            connection_close(connection_t *conn);
int            connection_destroy(connection_t *conn);
int            connection_run(connection_t *conn);
int            connection_finish_current_request(connection_t *conn);
void           connection_run_handler(connection_t *conn, handler_func handler);

server_t*      server_create();
server_t*      server_parse_conf(char *confile);
int            server_destroy(server_t *server);
int            server_start(server_t *server);
int            server_stop(server_t *server);

/* Common HTTP status codes */

// 1xx informational
extern http_status_t STATUS_CONTINUE;

// 2xx success
extern http_status_t STATUS_OK;
extern http_status_t STATUS_CREATED;
extern http_status_t STATUS_ACCEPTED;
extern http_status_t STATUS_NO_CONTENT;
extern http_status_t STATUS_PARTIAL_CONTENT;

// 3xx redirection
extern http_status_t STATUS_MOVED;
extern http_status_t STATUS_FOUND;
extern http_status_t STATUS_SEE_OTHER;
extern http_status_t STATUS_NOT_MODIFIED;

// 4xx client errors
extern http_status_t STATUS_BAD_REQUEST;
extern http_status_t STATUS_UNAUTHORIZED;
extern http_status_t STATUS_FORBIDDEN;
extern http_status_t STATUS_NOT_FOUND;
extern http_status_t STATUS_METHOD_NOT_ALLOWED;
extern http_status_t STATUS_RANGE_NOT_SATISFIABLE;

// 5xx server errors
extern http_status_t STATUS_INTERNAL_ERROR;
extern http_status_t STATUS_NOT_IMPLEMENTED;
extern http_status_t STATUS_BAD_GATEWAY;
extern http_status_t STATUS_SERVICE_UNAVAILABLE;
extern http_status_t STATUS_GATEWAY_TIMEOUT;


typedef enum {
    HTTP_VERSION_UNKNOW = -1,
    HTTP_VERSION_0_9 = 9,
    HTTP_VERSION_1_0 = 10,
    HTTP_VERSION_1_1 = 11
} http_version_e;

typedef enum _http_methods {
    HTTP_METHOD_EXTENDED = 0,
    HTTP_METHOD_GET = 1, 
    HTTP_METHOD_POST, 
    HTTP_METHOD_PUT, 
    HTTP_METHOD_DELETE,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_TRACE,
    HTTP_METHOD_CONNECT,
    HTTP_METHOD_OPTIONS
} http_method_e;

typedef enum _connection_opt {
    CONN_CLOSE = 0,
    CONN_KEEP_ALIVE
} connection_opt_e;

struct _http_header {
    char    *name;
    char    *value;
};

union _ctx_state {
    void      *as_ptr;
    long long as_long;
    int       as_int;
    char      *as_str;
};

struct _handler_ctx {
    ctx_state_t       _stat_stack[MAX_STATE_STACK_SIZE];
    int               _stat_top;
    void              *conf;
};

struct _request {
    char                    *path;
    char                    *query_str;
    char                    *method;
    http_version_e          version;

    char                    *host;
    size_t                  content_length;
    connection_opt_e        connection;

    char                    *body;

    // Unresolved headers of the request
    http_header_t           headers[MAX_HEADER_SIZE];
    size_t                  header_count;

    connection_t            *_conn;

    struct hsearch_data     _header_hash;

    // All the request header fields' are stored in this buffer 
    char                    _buffer[REQUEST_BUFFER_SIZE];
    int                     _buf_idx;
};

struct _http_status {
    unsigned int  code;
    char          *msg;
};

struct _response {
    http_status_t        status;
    http_version_e       version;
    long                 content_length;
    connection_opt_e     connection;
    int                  keep_alive_timeout;
    /*
     The response object does not manage the meory for the headers
     itself. So the programmer must guarantee that the pointers in
     the http_header_t struct is effective before the headers are
     sent to the client.
     */
    http_header_t        headers[MAX_HEADER_SIZE];
    size_t               header_count;

    struct hsearch_data  _header_hash;

    char                 _buffer[RESPONSE_BUFFER_SIZE];
    size_t               _buf_idx;
    int                  _header_sent;
    connection_t         *_conn;
    size_t               *_size_written;
    // This response is done?
    int                  _done;

    // Next handler to call after current write finishes
    handler_func         _next_handler;
};

typedef enum _server_state {
    SERVER_INIT = 0,
    SERVER_RUNNING,
    SERVER_STOPPED
} server_state;

struct _server {
    handler_func    handler;
    void            *handler_conf;
    json_value      *conf;

    server_state    state;
    int             listen_fd;
    ioloop_t        *ioloop;

    char            *addr;
    unsigned short   port;
};

typedef enum _connection_state {
    CONN_ACTIVE,
    CONN_CLOSING,
    CONN_CLOSED
} conn_stat_e;

struct _connection {
    server_t           *server;
    iostream_t         *stream;
    char               remote_ip[20];
    unsigned short     remote_port;
    conn_stat_e        state;

    request_t          *request;
    response_t         *response;
    handler_ctx_t      *context;
};


#endif /* end of include guard: __HTTP_H */
