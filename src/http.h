#ifndef __HTTP_H
#define __HTTP_H

#define REQUEST_BUFFER_SIZE     4096
#define MAX_HEADER_SIZE         25

#include <stddef.h>
#include <time.h>

struct _request;
struct _response;
struct _http_parser;

typedef struct _request         request_t;
typedef struct _response        response_t;
typedef struct _http_parser     http_parser_t;
typedef struct _http_header     http_header_t;

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

struct _request {
    char                    *path;
    char                    *query_str;
    char                    *method;
    http_version_e          version;

    int                     header_count;

    // Unresolved raw headers of the request
    http_header_t           raw_headers_in[MAX_HEADER_SIZE];
    int                     raw_header_count;

    // All the request header fields' are stored in this buffer 
    char                    _buffer_in[REQUEST_BUFFER_SIZE];
    int                     _buf_in_idx;
};

request_t*   request_create();
int          request_parse_headers(request_t *request,
                                   const char *data,
                                   const size_t data_len,
                                   size_t *consumed);

#endif /* end of include guard: __HTTP_H */
