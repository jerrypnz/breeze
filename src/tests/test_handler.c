#include "handler.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef struct {
    handler_t   vtbl;
    int         data;
} foo_handler;

typedef struct {
    handler_t   vtbl;
    char        data[64];
} bar_handler;

int foo_handler_handle(handler_t *handler, const char *path, request_t *req) {
    foo_handler  *this = (foo_handler*) handler;
    this->data = 0xCAFEBABE;
    printf("foo handler finished.\n");
    return 0;
}

foo_handler* create_foo_handler() {
    foo_handler *handler = (foo_handler*) malloc(sizeof(foo_handler));
    handler->vtbl.handle = foo_handler_handle;
    return handler;
}

int bar_handler_handle(handler_t *handler, const char *path, request_t *req) {
    bar_handler *this = (bar_handler*) handler;
    strncpy(this->data, "0xCAFEBABE", 64);
    printf("bar handler finished.\n");
    return 0;
}

bar_handler* create_bar_handler() {
    bar_handler *handler = (bar_handler*) malloc(sizeof(bar_handler));
    handler->vtbl.handle = bar_handler_handle;
    return handler;
}

filter_chain_t      chain;
foo_handler         *h1;
bar_handler         *h2;
filter_t            f1, f2;

void init_filter_chain() {
    bzero(&chain, sizeof(filter_chain_t));
    bzero(&f1, sizeof(filter_t));
    bzero(&f2, sizeof(filter_t));

    h1 = create_foo_handler();
    h2 = create_bar_handler();
    f1.handler = (handler_t*)h1;
    f2.handler = (handler_t*)h2;

    filter_chain_add(&chain, &f1);
    filter_chain_add(&chain, &f2);
    assert(chain.head != NULL);
    assert(chain.head->handler == (handler_t*)h1);
    assert(chain.tail != NULL);
    assert(chain.tail->handler == (handler_t*)h2);
}

void call_handlers() {
    filter_chain_do_filter(&chain, "test", NULL);
    printf("foo handler data: 0x%X\n", h1->data);
    printf("bar handler data: %s\n", h2->data);
    assert(h1->data == 0xCAFEBABE);
    assert(strcmp("0xCAFEBABE", h2->data) == 0);
}

int main(int argc, const char *argv[]) {
    init_filter_chain();
    call_handlers();
    return 0;
}
