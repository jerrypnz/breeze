#include "http.h"
#include "site.h"
#include "stacktrace.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    server_t    *server;
    site_conf_t *site_conf;
    site_t      *site;
    location_t  *loc;
    int i;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s config_file\n", argv[0]);
    }
    print_stacktrace_on_error();
    server = server_parse_conf(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "Error parsing config file\n");
        return 1;
    }

    site_conf = (site_conf_t*) server->handler_conf;
    for (i = 0; i < site_conf->site_size; i++) {
        site = site_conf->sites[i];
        printf("Found site: %s\n", site->host);
        for (loc = site->location_head->next; loc != NULL; loc = loc->next) {
            printf("\t Location: ");
            if (loc->match_type == URI_REGEX) {
                printf("[regex]\n");
            } else if (loc->match_type == URI_PREFIX) {
                printf("%s\n", loc->uri.prefix);
            }
        }
    }
    return 0;
}


