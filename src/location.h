#ifndef __LOCATION_H
#define __LOCATION_H

#include "handler.h"

#define MAX_HOST_LENGTH     64
#define MAX_SITES           10
#define MAX_LOCATIONS       50

typedef struct _location        location_t;
typedef struct _site            site_t;

struct _location {
    char                *path;
    handler_t           *handler;
    filter_chain_t      *filter_chain;
};

struct _site {
    char                    host[MAX_HOST_LENGTH];
    location_t              *locations[MAX_LOCATIONS];
    int                     loc_size;
};

site_t      *site_create(const char* host);
int         site_destroy(site_t *site);
int         site_add_location(site_t *site, location_t *loc);
location_t  *site_find_location(site_t *site, const char *path);

#endif /* end of include guard: __LOCATION_H */
