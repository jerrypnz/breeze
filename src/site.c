#include "site.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

site_conf_t *site_conf_create() {
    site_conf_t  *conf;

    conf = (site_conf_t*) calloc(1, sizeof(site_conf_t));
    if (conf == NULL) {
        fprintf(stderr, "Error allocating memory for site configuration");
        return NULL;
    }

    if (hcreate_r(MAX_SITES, &conf->site_hash) == 0) {
        perror("Error creating site hash");
        free(conf);
        return NULL;
    }

    return conf;
}


int site_conf_destroy(site_conf_t *conf) {
    hdestroy_r(&conf->site_hash);
    free(conf);
    return 0;
}


int site_conf_add_site(site_conf_t *conf, site_t *site) {
    char *host = site->host;
    ENTRY item, *ret;

    if (conf->site_size >= MAX_SITES) {
        fprintf(stderr, "Max sites reached: %d", MAX_SITES);
        return -1;
    }

    item.key = host;
    if (hsearch_r(item, FIND, &ret, &conf->site_hash) != 0) {
        fprintf(stderr, "Duplicate host: %s\n", host);
        return -1;
    }

    conf->sites[conf->site_size++] = site;
    
    item.data = site;
    hsearch_r(item, ENTER, &ret, &conf->site_hash);
    return 0;
}


site_t *site_create(const char* host) {
    site_t     *site;
    location_t *loc;

    site = (site_t*) calloc(1, sizeof(site_t));
    if (site == NULL) {
        fprintf(stderr, "Error allocating memory for the site\n");
        return NULL;
    }

    strncpy(site->host, host, MAX_HOST_LENGTH);
    loc = (location_t*) calloc(1, sizeof(location_t));
    if (loc == NULL) {
        fprintf(stderr, "Error allocating memory for location\n");
        free(site);
        return NULL;
    }

    site->location_head = loc;
    return site;
}


int site_destroy(site_t *site) {
    location_t *prev, *loc = site->location_head;
    
    while (loc != NULL) {
        prev = loc;
        loc = loc->next;
        free(prev);
    }
    free(site);
    return 0;
}


int site_handler(request_t *req, response_t *resp, handler_ctx_t *ctx) {
    return response_send_status(resp, STATUS_NOT_FOUND);
}


int site_add_location(site_t *site, int type,
                      char *prefix_or_regex,
                      handler_func *handler,
                      void *handler_conf) {
    location_t  *loc = NULL, *pos;
    regex_t     *reg;
    void        *mem;
    size_t      len;

    pos = site->location_head;
    len = strlen(prefix_or_regex);

    if (len == 0) {
        fprintf(stderr, "Empty prefix/regex\n");
        return -1;
    }
    if (type == URI_PREFIX) {
        if (prefix_or_regex[0] != '/') {
            fprintf(stderr, "URI Prefix must starts with /\n");
            return -1;
        }
        loc = (location_t*) calloc(1, sizeof(location_t));
        if (loc == NULL) {
            fprintf(stderr, "Error allocating memory for new location\n");
            return -1;
        }
        loc->uri.prefix = prefix_or_regex;        
    } else if (type == URI_REGEX) {
        mem = calloc(1, sizeof(location_t) + sizeof(regex_t));
        if (mem != NULL) {
            loc = (location_t*) mem;
            mem = ((char*)mem + sizeof(location_t));
            reg = (regex_t*) mem;
        } else {
            fprintf(stderr, "Error allocating memory for new location\n");
            return -1;
        }
        if (regcomp(reg, prefix_or_regex, REG_EXTENDED) != 0) {
            fprintf(stderr, "Invalid regex: %s\n", prefix_or_regex);
            free(loc);
            return -1;
        }
        loc->uri.regex = reg;
    } else {
        fprintf(stderr, "Unknown location type: %d\n", type);
        return -1;
    }
    
    loc->match_type = type;
    loc->handler = handler;
    loc->handler_conf = handler_conf;

    while (pos->next != NULL) {
        if (type == URI_REGEX
            && pos->next->match_type != URI_REGEX) {
            break;
        } else if (type == URI_PREFIX
                   && pos->next->match_type == URI_PREFIX
                   && len > strlen(pos->next->uri.prefix)) {
            break;
        }
        pos = pos->next;
    }
    
    loc->next = pos->next;
    pos->next = loc;
    return 0;
}


