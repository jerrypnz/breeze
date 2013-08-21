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
    hdestroy_r(conf->site_hash);
    free(conf);
    return 0;
}


int site_conf_add_site(site_conf_t *conf, site_t *site) {
    return 0;
}


site_t *site_create(const char* host) {
    site_t  *site;

    site = (site_t*) malloc(sizeof(site_t));
    if (site == NULL) {
        fprintf(stderr, "Error allocating memory for the site");
        return NULL;
    }

    bzero(site, sizeof(site_t));
    strncpy(site->host, host, MAX_HOST_LENGTH);

    return site;
}


int site_destroy(site_t *site) {
    free(site);
    return 0;
}


int site_handler(request_t *req, response_t *resp, handler_ctx_t *ctx) {
    return response_send_status(resp, STATUS_NOT_FOUND);
}


int site_add_location(site_t *site, int type,
                      const char *prefix_or_regex,
                      handler_t *handler void *handler_conf) {
    return 0;
}


