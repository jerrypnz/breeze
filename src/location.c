#include "location.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int _url_starts_with(const char* str, const char* prefix);


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


int site_add_location(site_t *site, location_t *loc) {
    assert(site != NULL);

    if (loc ==  NULL) {
        fprintf(stderr, "The location object is NULL\n");
        return -1;
    }

    if (site->loc_size >= MAX_LOCATIONS) {
        fprintf(stderr, "The site's location table is full\n");
        return -1;
    }

    //TODO: Check the validity of the location's path

    site->locations[site->loc_size] = loc;
    site->loc_size++;

    return 0;
}


static int _url_starts_with(const char* str, const char* prefix) {
    int match_count;

    if (str == NULL || prefix ==  NULL)
        return 0;

    match_count = 0;

    while (*str != '\0' && *prefix != '\0' && *str == *prefix) {
        match_count++;
        str++;
        prefix++;
    }

    if (*prefix == '\0' && (*str == '\0' || *str == '/')) 
        return match_count; 
    else 
        return 0;
}


location_t *site_find_location(site_t *site, const char *path) {
    int         i, max_matched_len, len;
    location_t  *matched_loc, *loc;

    max_matched_len = 0;
    matched_loc = NULL;

    for (i = 0; i < site->loc_size; i++) {
        loc = site->locations[i];
        len = _url_starts_with(path, loc->path);

        if (len > max_matched_len) {
            max_matched_len = len;
            matched_loc = loc;
        }
    }

    return matched_loc;
}
