#ifndef __LOCATION_H
#define __LOCATION_H

#include "http.h"
#include <regex.h>
#include <search.h>

#define MAX_HOST_LENGTH     64
#define MAX_SITES           100

typedef struct _location        location_t;
typedef struct _site            site_t;

enum _location_match_type {
    URI_PREFIX,
    URI_REGEX
};

struct _location {
    union {
        char     *prefix;
        regex_t  *regex;
    } uri;
    int                 match_type;
    handler_func        *handler;
    void                *handler_conf;
    struct _location    *next;
};

struct _site {
    char                    host[MAX_HOST_LENGTH];
    
    /*
     * Use a single linked list to hold all the locations. The regex
     * locations have higher priority, so they are always at the front
     * of the list. There order is the samed as the order in the
     * config file.
     *
     * After regex locations goes the normal URI prefix
     * locations. These locations, however, are sorted in a
     * longest-to-shortest order.
     *
     * Using this structure, we can stop at
     * the first match and it is guaranteed to be the best match.
     */
    location_t              *location_head;
};

typedef struct _site_conf {
    site_t               *sites[MAX_SITES];
    int                  site_size;
    struct hsearch_data  site_hash;
} site_conf_t;

int         site_handler(request_t *req, response_t *resp, handler_ctx_t *ctx);

site_conf_t *site_conf_create();
int          site_conf_destroy(site_conf_t *conf);
int          site_conf_add_site(site_conf_t *conf, site_t *site);

site_t      *site_create(const char* host);
int          site_destroy(site_t *site);

int         site_add_location(site_t *site, int type,
                              char *prefix_or_regex,
                              handler_func *handler,
                              void      *handler_conf);

#endif /* end of include guard: __LOCATION_H */
