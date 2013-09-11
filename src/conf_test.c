#include "http.h"
#include "site.h"
#include "stacktrace.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    server_t    *server;
    site_conf_t *site_conf;
    site_t      *site;
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
    }
    return 0;
}


