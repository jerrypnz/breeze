#ifndef __HTTP_H
#define __HTTP_H

#define _GNU_SOURCE

#include "iostream.h"
#include <stddef.h>
#include <time.h>
#include <search.h>

#define REQUEST_BUFFER_SIZE     2048
#define RESPONSE_BUFFER_SIZE    2048
#define MAX_HEADER_SIZE         25

struct _request;
struct _response;
struct _ctx_node;
struct _handler_ctx;
struct _http_status;

typedef struct _request         request_t;
typedef struct _response        response_t;
typedef struct _http_header     http_header_t;
typedef struct _http_status     http_status_t;
typedef struct _handler_ctx     handler_ctx_t;
typedef struct _ctx_node        ctx_node_t;

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

struct _ctx_node {
    union {
        void    *as_ptr;
        size_t  as_size;
        int     as_int;
        char    *as_str;
    } data;
    struct _ctx_node *next;
};

struct _handler_ctx {
    ctx_node_t   *head;
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

    iostream_t              *_stream;

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
    iostream_t           *_stream;
};

/*
 * HTTP handler function
 */
typedef int (*handler_func)(request_t *request, response_t *response, handler_ctx_t *ctx);

request_t*   request_create();
int          request_destroy(request_t *request);
const char*  request_get_header(request_t *request, const char *header_name);
int          request_parse_headers(request_t *request,
                                   const char *data,
                                   const size_t data_len,
                                   size_t *consumed);

response_t*    response_create();
int            response_destroy(response_t *response);
const char*    response_get_header(response_t *response, const char *header_name);
int            response_set_header(response_t *response, char *name, char *value);
int            response_write(response_t *response, handler_func next_handler);


#endif /* end of include guard: __HTTP_H */
