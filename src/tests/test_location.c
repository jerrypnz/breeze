#include "location.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

static site_t   *site;

static location_t loc1 = {
    "/foo",
    NULL,
    NULL
};

static location_t loc2 = {
    "/foo/bar",
    NULL,
    NULL
};

static location_t loc3 = {
    "/bar",
    NULL,
    NULL
};

void test_create_site() {
    site = site_create("www.foobar.com");
    assert(site != NULL);
    assert(strcmp("www.foobar.com", site->host) == 0);
    assert(site->loc_size == 0);
}


void test_add_location() {
    assert(site_add_location(site, &loc1) == 0);
    assert(site_add_location(site, &loc2) == 0);
    assert(site_add_location(site, &loc3) == 0);

    assert(site->loc_size == 3);
    assert(site->locations[0] == &loc1);
    assert(site->locations[1] == &loc2);
    assert(site->locations[2] == &loc3);
}


void test_find_location() {
    assert(site_find_location(site, "/foo") == &loc1);
    assert(site_find_location(site, "/foo/aaa") == &loc1);
    assert(site_find_location(site, "/fooaa") == NULL);

    assert(site_find_location(site, "/foo/bar") == &loc2);
    assert(site_find_location(site, "/foo/bar/hhh") == &loc2);

    assert(site_find_location(site, "/bar") == &loc3);
    assert(site_find_location(site, "/barr") == NULL);

    assert(site_find_location(site, "/xxxxx") == NULL);
}


int main(int argc, const char *argv[]) {
    test_create_site();
    test_add_location();
    test_find_location();
    return 0;
}

