#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void dump_request(request_t *req) {
    int i;

    printf("--------- Parser State -----------------------\n");
    printf("Method: %s\n", req->method);
    printf("Path: %s\n", req->path);
    printf("Query String: %s\n", req->query_str);
    printf("HTTP Version: %d\n", req->version);
    printf("Header count: %zu\n", req->header_count);
    printf("Headers: \n");
    printf("------------\n");

    for (i = 0; i < req->header_count; i++) {
        printf("\r%s: %s\n", req->headers[i].name, req->headers[i].value);
    }

    printf("----------------------------------------------\n");
}

void write_finished(iostream_t *stream) {
    printf("finished request\n");
    iostream_close(stream);
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
    char buffer[2048];

    snprintf(buffer,
             2048,
             "HTTP/1.1 200 OK\r\n"
             "Connection: close\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %zu\r\n"
             "\r\n"
             "%s",
             strlen(response),
             response);
    
    dump_request(req);
    iostream_write(req->_conn->stream, buffer, strlen(buffer), write_finished);
    return 0;
}

int main(int argc, char** args) {
    server_t *server;
    server = server_create(8000, NULL);

    if (server == NULL) {
        fprintf(stderr, "Error creating server\n");
        return -1;
    }

    server->handler = foobar_handler;
    server_start(server);
    return 0;
}
