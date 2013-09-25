#include "site.h"
#include "stacktrace.h"
#include "log.h"
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

#define ROOT_VAL 90
#define FOO_VAL  91
#define BAR_VAL  92
#define FOOBAR_VAL 93
#define JPG_VAL    94

int root_handler(request_t *req, response_t *resp, handler_ctx_t *ctx) {
    return ROOT_VAL;
}

int foo_handler(request_t *req, response_t *resp, handler_ctx_t *ctx) {
    return FOO_VAL;
}

int bar_handler(request_t *req, response_t *resp, handler_ctx_t *ctx) {
    return BAR_VAL;
}

int foobar_handler(request_t *req, response_t *resp, handler_ctx_t *ctx) {
    return FOOBAR_VAL;
}

int jpg_handler(request_t *req, response_t *resp, handler_ctx_t *ctx) {
    return JPG_VAL;
}

void test_handler_single_site() {
    site_conf_t  *conf = site_conf_create();
    site_t *site = site_create("jerrypeng.me");
    request_t req;
    response_t resp;
    handler_ctx_t ctx;

    bzero(&req, sizeof(request_t));
    bzero(&resp, sizeof(response_t));
    bzero(&ctx, sizeof(handler_ctx_t));

    assert(site_conf_add_site(conf, site) == 0);
    assert(site_add_location(site, URI_PREFIX, "/", root_handler, NULL) == 0);
    assert(site_add_location(site, URI_PREFIX, "/foo", foo_handler, NULL) == 0);
    assert(site_add_location(site, URI_PREFIX, "/bar", bar_handler, NULL) == 0);
    assert(site_add_location(site, URI_PREFIX, "/foo/bar", foobar_handler, NULL) == 0);
    assert(site_add_location(site, URI_REGEX, "\\.jpg", jpg_handler, NULL) == 0);

    req.host = "jerrypeng.me";

    ctx.conf = conf;
    req.path = "/foo/bar";
    assert(site_handler(&req, &resp, &ctx) == FOOBAR_VAL);

    ctx.conf = conf;
    req.path = "/foo/bar1";
    assert(site_handler(&req, &resp, &ctx) == FOO_VAL);
    
    ctx.conf = conf;
    req.path = "/bar/1";
    assert(site_handler(&req, &resp, &ctx) == BAR_VAL);
    
    ctx.conf = conf;
    req.path = "/foo/bar/foo.jpg";
    assert(site_handler(&req, &resp, &ctx) == JPG_VAL);

    ctx.conf = conf;    
    req.path = "/foo.jpg";
    assert(site_handler(&req, &resp, &ctx) == JPG_VAL);

    ctx.conf = conf;    
    req.path = "/";
    assert(site_handler(&req, &resp, &ctx) == ROOT_VAL);

}


void test_handler_multi_site() {
    site_conf_t  *conf = site_conf_create();
    site_t *site = site_create("foo.com");
    site_t *site1 = site_create("bar.com");
    request_t req;
    response_t resp;
    handler_ctx_t ctx;

    bzero(&req, sizeof(request_t));
    bzero(&resp, sizeof(response_t));
    bzero(&ctx, sizeof(handler_ctx_t));

    assert(site_add_location(site, URI_PREFIX, "/", foo_handler, NULL) == 0);
    assert(site_conf_add_site(conf, site) == 0);

    assert(site_add_location(site1, URI_PREFIX, "/", bar_handler, NULL) == 0);
    assert(site_conf_add_site(conf, site1) == 0);

    req.host = "jerrypeng.me"; // Unknown host will go to the first
                               // one configured.
    ctx.conf = conf;
    req.path = "/foo/bar";
    assert(site_handler(&req, &resp, &ctx) == FOO_VAL);

    req.host = "foo.com";
    ctx.conf = conf;
    req.path = "/foo/bar";
    assert(site_handler(&req, &resp, &ctx) == FOO_VAL);

    req.host = "bar.com";
    ctx.conf = conf;
    req.path = "/";
    assert(site_handler(&req, &resp, &ctx) == BAR_VAL);
}

int main(int argc, char *argv[]) {
    print_stacktrace_on_error();
    test_create_site_conf();
    test_create_site();
    test_add_site();
    test_add_location();
    test_handler_single_site();
    test_handler_multi_site();
    info("All tests finished.");
    return 0;
}
