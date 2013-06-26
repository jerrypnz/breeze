#ifndef __HTTP_H
#define __HTTP_H

#define _GNU_SOURCE

#include <stddef.h>
#include <time.h>
#include <search.h>

#define REQUEST_BUFFER_SIZE     2048
#define MAX_HEADER_SIZE         25

struct _request;
struct _response;

typedef struct _request         request_t;
typedef struct _response        response_t;
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

    // Unresolved headers of the request
    http_header_t           headers[MAX_HEADER_SIZE];
    size_t                  header_count;

    struct hsearch_data     _header_hash;

    // All the request header fields' are stored in this buffer 
    char                    _buffer_in[REQUEST_BUFFER_SIZE];
    int                     _buf_in_idx;
};

request_t*   request_create();
int          request_destroy(request_t *request);
int          request_parse_headers(request_t *request,
                                   const char *data,
                                   const size_t data_len,
                                   size_t *consumed);

#endif /* end of include guard: __HTTP_H */
