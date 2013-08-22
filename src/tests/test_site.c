#include "site.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void test_create_site_conf() {
    site_conf_t  *conf = site_conf_create();
    assert(conf != NULL);
    assert(conf->site_size == 0);
    assert(site_conf_destroy(conf) == 0);
}

static void test_create_site() {
    site_t *site = site_create("jerrypeng.me");
    assert(site != NULL);
    assert(strcmp("jerrypeng.me", site->host) == 0);
    assert(site->location_head != NULL);
    assert(site->location_head->next == NULL);
    assert(site_destroy(site) == 0);
}

static void test_add_site() {
    site_conf_t  *conf = site_conf_create();
    site_t *site = site_create("jerrypeng.me");
    site_t *site2 = site_create("clojure.org");
    site_t *site3 = site_create("jerrypeng.me");

    assert(site_conf_add_site(conf, site) == 0);
    assert(site_conf_add_site(conf, site2) == 0);
    // Duplicate site can not be added.
    assert(site_conf_add_site(conf, site3) < 0);

    assert(conf->site_size == 2);
    assert(conf->sites[0] != NULL);
    assert(strcmp("jerrypeng.me", conf->sites[0]->host) == 0);
    assert(conf->sites[1] != NULL);
    assert(strcmp("clojure.org", conf->sites[1]->host) == 0);
}

static void test_add_location() {
    site_t *site = site_create("jerrypeng.me");
    location_t *loc;
    
    assert(site_add_location(site, URI_PREFIX, "/", NULL, NULL) == 0);
    assert(site_add_location(site, URI_PREFIX, "/foo", NULL, NULL) == 0);
    assert(site_add_location(site, URI_PREFIX, "/foobar", NULL, NULL) == 0);
    assert(site_add_location(site, URI_PREFIX, "/bar", NULL, NULL) == 0);
    assert(site_add_location(site, URI_PREFIX, "/foo/bar", NULL, NULL) == 0);

    // Prefix must starts with "/"
    assert(site_add_location(site, URI_PREFIX, "oops", NULL, NULL) < 0);
    
    assert(site_add_location(site, URI_REGEX, "\\.jsp", NULL, "jsp") == 0);
    assert(site_add_location(site, URI_REGEX, "\\.php", NULL, "php") == 0);
    assert(site_add_location(site, URI_REGEX, "\\.cgi", NULL, "cgi") == 0);

    //Invalid regex
    assert(site_add_location(site, URI_REGEX, "*", NULL, NULL) < 0);

    loc = site->location_head->next;

    assert(loc != NULL);
    assert(loc->match_type == URI_REGEX);
    assert(strcmp("jsp", (char*)loc->handler_conf) == 0);
    loc = loc->next;

    assert(loc != NULL);
    assert(loc->match_type == URI_REGEX);
    assert(strcmp("php", (char*)loc->handler_conf) == 0);
    loc = loc->next;

    assert(loc != NULL);
    assert(loc->match_type == URI_REGEX);
    assert(strcmp("cgi", (char*)loc->handler_conf) == 0);
    loc = loc->next;

    assert(loc != NULL);
    assert(loc->match_type == URI_PREFIX);
    assert(strcmp("/foo/bar", loc->uri.prefix) == 0);
    loc = loc->next;

    assert(loc != NULL);
    assert(loc->match_type == URI_PREFIX);
    assert(strcmp("/foobar", loc->uri.prefix) == 0);
    loc = loc->next;

    assert(loc != NULL);
    assert(loc->match_type == URI_PREFIX);
    assert(strcmp("/foo", loc->uri.prefix) == 0);
    loc = loc->next;

    assert(loc != NULL);
    assert(loc->match_type == URI_PREFIX);
    assert(strcmp("/bar", loc->uri.prefix) == 0);
    loc = loc->next;

    assert(loc != NULL);
    assert(loc->match_type == URI_PREFIX);
    assert(strcmp("/", loc->uri.prefix) == 0);
    loc = loc->next;
}

int main(int argc, char *argv[]) {
    test_create_site_conf();
    test_create_site();
    test_add_site();
    test_add_location();
    printf("All tests finished.\n");
    return 0;
}
