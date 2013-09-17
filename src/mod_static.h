#ifndef __MOD_STATIC_H_
#define __MOD_STATIC_H_

#include "http.h"
#include "mod.h"

typedef struct _mod_static_conf {
    char *root;
    char *index;
    int  enable_list_dir;
    int  show_hidden_file;
    int  enable_etag;
    int  enable_range_req;

    // -1 means not set expires header; other means
    // the expiration time (in hours)
    int  expire_hours;
} mod_static_conf_t;

extern module_t mod_static;

int mod_static_init();
int static_file_handle(request_t *req, response_t *resp, handler_ctx_t *ctx);

#endif /* __MOD_STATIC_H_ */










