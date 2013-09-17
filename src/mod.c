#include "mod.h"
#include "mod_static.h"
#include "http.h"
#include <string.h>
#include <stdio.h>

static module_t *modules[] = {
    &mod_static
};

static int modules_inited = 0;

module_t *find_module(const char* name) {
    int i;
    size_t size;

    size = sizeof(modules)/sizeof(modules[0]);

    for (i = 0; i < size; i++) {
        if (strcmp(name, modules[i]->name) == 0) {
            return modules[i];
        }
    }

    return NULL;
}

int init_modules() {
    int i;
    size_t size;
    module_t *mod;

    if (modules_inited) {
        return 0;
    }
    
    size = sizeof(modules)/sizeof(modules[0]);

    for (i = 0; i < size; i++) {
        mod = modules[i];
        if (mod->init != NULL) {
            printf("Init module: %s\n", mod->name);
            if (mod->init() != 0) {
                fprintf(stderr, "Error initializing module: %s\n", mod->name);
                return -1;
            }
        }
    }

    modules_inited = 1;
    return 0;
}
