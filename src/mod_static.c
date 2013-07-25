#include "http.h"
#include <unistd.h>

static int static_file_write_content(request_t *req, request_t *resp, handler_ctx_t *ctx);

int static_file_handle(request_t *req, response_t *resp, handler_ctx_t *ctx) {
    return HANDLER_UNFISHED;
}

static int static_file_write_content(request_t *req, request_t *resp, handler_ctx_t *ctx) {
    return HANDLER_DONE;
}
