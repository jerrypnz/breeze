#include "http.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

typedef struct _mod_static_conf {
    char root[1024];
    int  enable_list_dir;
    int  enable_etag;
    int  enable_range_req;
    int  enable_last_modified;
    int  enable_cache;
    int  cache_age;
} mod_static_conf_t;

/*
typedef struct _mime_type {
    char *content_type;
    char **file_types;
} mime_type_t;

mime_type_t standard_types = {
    {"text/html", {"html", "htm", "shtml"}},
    {"text/css", {"css"}},
    {"text/xml", {"xml"}},
    {"text/plain", {"txt"}},
    {"image/png", {"png"}},
    {"image/gif", {"gif"}},
    {"image/tiff", {"tif", "tiff"}},
    {"image/jpeg", {"jpg", "jpeg"}},
    {"image/x-ms-bmp", {"bmp"}},
    {"image/svg+xml", {"svg", "svgz"}},
    {"application/x-javascript", {"js"}},
    };
*/

static int static_file_write_content(request_t *req, response_t *resp, handler_ctx_t *ctx);
static int static_file_cleanup(request_t *req, response_t *resp, handler_ctx_t *ctx);
static int static_file_handler_error(response_t *resp);

int static_file_handle(request_t *req, response_t *resp, handler_ctx_t *ctx) {
    mod_static_conf_t *conf;
    char              path[2048];
    int               fd, res;
    struct stat       st;
    size_t            len;
    ctx_state_t       val;

    conf = (mod_static_conf_t*) ctx->conf;
    len = strlen(conf->root);
    strncpy(path, conf->root, 2048);
    if (req->path[0] != '/') {
        return response_send_status(resp,
                                    STATUS_BAD_REQUEST,
                                    "Request path must starts with /");
    }
    strncat(path, req->path, 2048 - len);
    printf("Request path: %s, real file path: %s\n", req->path, path);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Resource not found");
        return static_file_handler_error(resp);
    }
    res = fstat(fd, &st);
    if (res < 0) {
        perror("Error fstat");
        return static_file_handler_error(resp);
    }
    resp->status = STATUS_OK;
    resp->connection = req->connection;
    resp->content_length = st.st_size;
    // TODO Implement MIME type mapping
    response_set_header(resp, "Content-Type", "application/octet-stream");
    response_set_header(resp, "Server", "breeze/0.1.0");

    val.as_int = fd;
    context_push(ctx, val);
    val.as_long = st.st_size;
    context_push(ctx, val);
    printf("sending headers\n");
    response_send_headers(resp, static_file_write_content);
    return HANDLER_UNFISHED;
}

static int static_file_write_content(request_t *req, response_t *resp, handler_ctx_t *ctx) {
    int fd;
    size_t size;

    size = context_pop(ctx)->as_long;
    fd = context_peek(ctx)->as_int;
    printf("writing file\n");
    if (response_send_file(resp, fd, 0, size, static_file_cleanup) < 0) {
        fprintf(stderr, "Error sending file\n");
        return response_send_status(resp, STATUS_NOT_FOUND,
                                    "Error sending file to client");
    }
    return HANDLER_UNFISHED;
}

static int static_file_cleanup(request_t *req, response_t *resp, handler_ctx_t *ctx) {
    int fd;

    printf("cleaning up\n");
    fd = context_pop(ctx)->as_int;
    close(fd);
    return HANDLER_DONE;
}

static int static_file_handler_error(response_t *resp) {
    switch(errno) {
    case EACCES:
    case EISDIR:
        return response_send_status(resp, STATUS_FORBIDDEN, "Access Denied");
    case ENOENT:
    default:
        return response_send_status(resp, STATUS_NOT_FOUND,
                                    "Requested resource not found");
    }
}

int main(int argc, char** args) {
    server_t *server;
    mod_static_conf_t conf;
    
    server = server_create(8000, NULL);

    if (server == NULL) {
        fprintf(stderr, "Error creating server\n");
        return -1;
    }

    if (argc < 2) {
        fprintf(stderr, "Usage: %s root_dir", args[0]);
        return -2;
    }

    strncpy(conf.root, args[1], 1024);
    server->handler = static_file_handle;
    server->handler_conf = &conf;
    server_start(server);
    return 0;
}
