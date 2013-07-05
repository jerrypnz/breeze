#include "connection.h"
#include "iostream.h"
#include "http.h"

connection_t* connection_create(server_t *server, iostream_t *stream) {
    return NULL;
}

int connection_close(connection_t *conn) {
    return 0;
}

int connection_destroy(connection_t *conn) {
    return 0;
}

int connection_run(connection_t *conn) {
    return 0;
}

int connection_finish_current_request(connection_t *conn) {
    return 0;
}


static void _on_http_header_data(iostream_t *stream, void *data, size_t len) {
    request_t      *req;
    response_t     *resp;
    handler_ctx_t  *ctx;
    size_t         consumed;
    // TODO Implementation

    ctx = NULL;
    req = request_create();
    resp = response_create();

    if (req == NULL || resp == NULL) {
        fprintf(stderr, "Error creating request/response\n");
        iostream_close(stream);
        return;
    }

    if (request_parse_headers(req, (char*)data, len, &consumed) != STATUS_COMPLETE) {
        fprintf(stderr, "Error parsing request headers\n");
        iostream_close(stream);
        return;
    }

    req->_stream = stream;
    resp->_stream = stream;
    
}
