#ifndef __MODULE_H_
#define __MODULE_H_

#include "http.h"
#include "json.h"

typedef void* (*mod_conf_create)(json_value *conf_value);
typedef int   (*mod_init)();

typedef struct _module {
    char            *name;
    mod_init         init;
    mod_conf_create  create;
    handler_func     handler;
} module_t;

module_t  *find_module(const char* name);
int        init_modules();

#endif /* __MODULE_H_ */










