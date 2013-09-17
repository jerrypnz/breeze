#include "http.h"
#include "mod.h"
#include "site.h"
#include "mod_static.h"
#include "stacktrace.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define DEFAULT_CONF "/etc/breeze.conf"

typedef struct _opts {
    char *conf_file;
    int  is_simple_mode;
    char *root_dir;
    unsigned short port;
    int  is_conf_test;
} opt_t;

static opt_t    *parse_opts(int argc, char *argv[]);
static server_t *create_simple_server(opt_t *opt);
static void     print_conf_details(server_t *server);

int main(int argc, char *argv[]) {
    server_t    *server;
    opt_t       *opt;

    opt = parse_opts(argc, argv);
    if (opt == NULL) {
        return -1;
    }
    print_stacktrace_on_error();
    // Simple Mode: do not need config file
    if (opt->root_dir != NULL) {
        server = create_simple_server(opt);
    } else {
        server = server_parse_conf(opt->conf_file);
    }
    
    if (server == NULL) {
        fprintf(stderr, "Error creating server\n");
        return 1;
    }

    if (opt->is_conf_test) {
        print_conf_details(server);
        return 0;
    }
    server_start(server);
    server_destroy(server);
    free(opt);
    return 0;
}

static opt_t *parse_opts(int argc, char *argv[]) {
    opt_t  *opt = NULL;
    int opt_id;
    
    opt = (opt_t*) calloc(1, sizeof(opt_t));
    if (opt == NULL) {
        fprintf(stderr, "Error allocating memory\n");
        return NULL;
    }
    opt->conf_file = DEFAULT_CONF;

    while ((opt_id = getopt(argc, argv, "c:p:r:t")) != -1) {
        switch(opt_id) {
        case 'c':
            opt->conf_file = optarg;
            break;

        case 'p':
            opt->port = atoi(optarg);
            break;

        case 't':
            opt->is_conf_test = 1;
            break;

        case 'r':
            opt->root_dir = optarg;
            break;

        default:
            fprintf(stderr, "Usage: %s [-c configfile] [-t]\n", argv[0]);
            fprintf(stderr, "       %s [-r rootdir] [-p port]\n", argv[0]);
            free(opt);
            return NULL;
        }
    }
    
    return opt;
}

static void print_conf_details(server_t *server) {
    int i;
    site_t      *site;
    location_t  *loc;
    site_conf_t *site_conf = (site_conf_t*) server->handler_conf;
    
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
}

mod_static_conf_t conf;

static server_t *create_simple_server(opt_t *opt) {
   server_t *server;
   bzero(&conf, sizeof(mod_static_conf_t));

   if (mod_static_init() < 0) {
       fprintf(stderr, "Error initializing mod_static\n");
       return NULL;
   }
   
   server = server_create();
   if (server == NULL) {
       return NULL;
   }

   conf.root = opt->root_dir;
   conf.expire_hours = 24;
   conf.enable_list_dir = 1;
   conf.index = "index.html";
   if (opt->port >= 0) {
       server->port = opt->port;
   }
   server->handler = static_file_handle;
   server->handler_conf = &conf;
   return server;
}

