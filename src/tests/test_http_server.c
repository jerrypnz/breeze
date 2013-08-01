#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

char* msg = "hello";

void dump_request(request_t *req) {
    int i;

    printf("--------- Parser State -----------------------\n");
    printf("Method: %s\n", req->method);
    printf("Path: %s\n", req->path);
    printf("Query String: %s\n", req->query_str);
    printf("HTTP Version: %d\n", req->version);
    printf("Header count: %zu\n", req->header_count);
    printf("Connection: %d\n", req->connection);
    printf("Headers: \n");
    printf("------------\n");

    for (i = 0; i < req->header_count; i++) {
        printf("\r%s: %s\n", req->headers[i].name, req->headers[i].value);
    }

    printf("----------------------------------------------\n");
}

int foobar_handler(request_t *req, response_t *resp, handler_ctx_t *ctx) {
    char *response =
        "<html>"
        "<head><title>Hello world</title></head>"
        "<body>"
        "<h1><center>Welcome to Breeze</center></h1>"
        "<p>Hello world!</p>"
        "</body>"
        "</html>";
    size_t len = strlen(response);
    
    dump_request(req);

    // Make sure the conf pointer points to the string we specified.
    assert(ctx->conf == (void*)msg);
    
    resp->status = STATUS_OK;
    resp->content_length = len;
    resp->connection = req->connection;
    response_set_header(resp, "Content-Type", "text/html");
    response_send_headers(resp, NULL);
    response_write(resp, response, len, NULL);
    return HANDLER_DONE;
}

int main(int argc, char** args) {
    server_t *server;
    server = server_create(8000, NULL);

    if (server == NULL) {
        fprintf(stderr, "Error creating server\n");
        return -1;
    }

    server->handler = foobar_handler;
    server->handler_conf = msg;
    server_start(server);
    return 0;
}
