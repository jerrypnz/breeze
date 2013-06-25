#include "handler.h"
#include <assert.h>

void filter_chain_add(filter_chain_t *chain, filter_t *ft) {
    assert(chain != NULL);
    if (chain->head ==  NULL) {
        chain->head = ft;
        chain->tail = ft;
    } else {
        chain->tail->next = ft;
        chain->tail = ft;
        ft->next = NULL;
    }
}

int filter_chain_do_filter(filter_chain_t *chain, const char* path, request_t *req) {
    int         rc;
    filter_t    *ft;

    for (ft = chain->head; ft != NULL; ft = ft->next) {
        rc = ft->handler->handle(ft->handler, path, req);
        if (rc != FT_CONTINUE)
            break;
    }

    return rc;
}
