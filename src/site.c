#include "common.h"
#include "site.h"
#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static site_t     *find_site(site_conf_t *conf, char *host);
static location_t *find_location(site_t *site, const char *path);

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

site_conf_t *site_conf_parse(json_value *sites_obj) {
    site_conf_t *conf;
    site_t      *site;
    json_value  *val;
    int i;

    if (sites_obj->type != json_array) {
        fprintf(stderr, "Config option 'sites' must be a JSON array\n");
        return NULL;
    }
    conf = site_conf_create();
    if (conf == NULL) {
        return NULL;
    }
    for (i = 0; i < sites_obj->u.array.length; i++) {
        val = sites_obj->u.array.values[i];
        if (val->type != json_object) {
            fprintf(stderr, "The elements of 'sites' must be JSON objects\n");
            site_conf_destroy(conf);
            return NULL;
        }
        site = site_parse(val);
        if (site == NULL) {
            fprintf(stderr, "Error creating site\n");
            site_conf_destroy(conf);
            return NULL;
        }
        site_conf_add_site(conf, site);
    }

    return conf;
}

int site_conf_destroy(site_conf_t *conf) {
    int i;
    for (i = 0; i < conf->site_size; i++) {
        site_destroy(conf->sites[i]);
    }
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

site_t *site_parse(json_value *site_obj) {
    site_t  *site;

    site = site_create(NULL);
    if (site == NULL) {
        return NULL;
    }

    // TODO Handle host, location settings..
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
    site_conf_t *conf;
    site_t      *site;
    location_t  *loc;

    conf = (site_conf_t*) ctx->conf;
    site = find_site(conf, req->host);
    if (site == NULL) {
        goto NOT_FOUND;
    }
    loc = find_location(site, req->path);
    if (loc == NULL) {
        goto NOT_FOUND;
    }

    ctx->conf = loc->handler_conf;
    return loc->handler(req, resp, ctx);

    NOT_FOUND:
    return response_send_status(resp, STATUS_NOT_FOUND);
}

int site_add_location(site_t *site, int type,
                      char *prefix_or_regex,
                      handler_func handler,
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
        if (regcomp(reg, prefix_or_regex, REG_EXTENDED | REG_NOSUB) != 0) {
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


static site_t *find_site(site_conf_t *conf, char *host) {
    ENTRY item, *ret;

    if (conf->site_size == 0) {
        return NULL;
    } else if (conf->site_size == 1) {
        return conf->sites[0];
    }

    item.key = host;
    if (hsearch_r(item, FIND, &ret, &conf->site_hash)) {
        return (site_t*) ret->data;
    } else {
        return conf->sites[0];
    }
}

static location_t *find_location(site_t *site, const char *path) {
    location_t *loc = site->location_head->next;

    while (loc != NULL) {
        switch (loc->match_type) {
        case URI_REGEX:
            if (regexec(loc->uri.regex, path, 0, NULL, 0) == 0)
                return loc;

        case URI_PREFIX:
            if (path_starts_with(loc->uri.prefix, path) > 0)
                return loc;
        }

        loc = loc->next;
    }

    return NULL;
}


