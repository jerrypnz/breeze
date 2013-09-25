#include "http.h"
#include "site.h"
#include "json.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>

#define MAX_CONNECTIONS 1024000
#define MAX_BACKLOG 128


static int _server_init(server_t *server);
static int _configure_server(server_t *server, json_value *conf_obj);
static void _server_connection_handler(ioloop_t *loop,
                                       int listen_fd,
                                       unsigned int events,
                                       void *args);


server_t* server_create() {
    server_t *server;
    ioloop_t *ioloop;

    server = (server_t*) calloc(1, sizeof(server_t));
    if (server == NULL) {
        error("Error allocating memory for server");
        return NULL;
    }

    ioloop = (ioloop_t*) ioloop_create(MAX_CONNECTIONS);
    if (ioloop == NULL) {
        error("Error creating ioloop");
        return NULL;
    }

    server->addr = "127.0.0.1";
    server->port = 8000;
    server->ioloop = ioloop;
    server->state = SERVER_INIT;
    server->loglevel = INFO;
    return server;
}


server_t* server_parse_conf(char *configfile) {
    server_t *server = NULL;
    json_value *json = NULL;
    json_settings settings = {0};
    int    fd;
    char   buf[10240];
    char   error[128];
    ssize_t sz;
    
    server = server_create();
    if (server == NULL) {
        return NULL;
    }

    fd = open(configfile, O_RDONLY);
    if (fd < 0) {
        error("Error opening config file");
        goto error;
    }
    sz = read(fd, buf, 10240);
    
    json = json_parse_ex(&settings, buf, sz, error);
    if (json == NULL) {
        error("Error parsing JSON config file: %s", error);
        goto error;
    } else if (json->type != json_object) {
        error("Invalid conf: the root is not a JSON object");
        goto error;
    }
    
    // This pointer is used for destroying the JSON value
    server->conf = json;
    
    if (_configure_server(server, json) != 0) {
        error("Error detected when configuring the server");
        goto error;
    }
    
    close(fd);
    return server;

    error:
    if (fd > 0) {
        close(fd);
    }
    if (json != NULL) {
        json_value_free(json);
        server->conf = NULL;
    }
    server_destroy(server);
    return NULL;
}

int server_destroy(server_t *server) {
    ioloop_destroy(server->ioloop);
    if (server->conf != NULL)
        json_value_free(server->conf);
    free(server);
    return 0;
}

int server_start(server_t *server) {
    configure_log(server->loglevel, server->logfile, !server->daemonize);
    if (_server_init(server) < 0) {
        error("Error initializing server");
        return -1;
    }
    // Block SIGPIPE
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        error("Error blocking SIGPIPE");
    }
    info("Start running server on %d", server->port);
    server->state = SERVER_RUNNING;
    return ioloop_start(server->ioloop);
}

int server_stop(server_t *server) {
    info("Stopping server");
    if (ioloop_stop(server->ioloop) < 0) {
        error("Error stopping ioloop");
        return -1;
    }
    server->state = SERVER_STOPPED;
    return 0;
}

static int _server_init(server_t *server) {
    int                     listen_fd;
    struct sockaddr_in      addr;

    // ---------- Create and bind listen socket fd --------------
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        error("Error creating socket");
        return -1;
    }

    bzero(&addr, sizeof(struct sockaddr_in));
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(server->port);

    if (bind(listen_fd,
             (struct sockaddr *)&addr,
             sizeof(struct sockaddr_in)) == -1) {
        error("Error binding address");
        close(listen_fd);
        return -1;
    }

    // ------------ Start listening ------------------------------
    if (listen(listen_fd, MAX_BACKLOG) == -1) {
        error("Error listening");
        close(listen_fd);
        return -1;
    }
    if (set_nonblocking(listen_fd) < 0) {
        error("Error configuring non-blocking");
        close(listen_fd);
        return -1;
    }
    server->listen_fd = listen_fd;
    if (ioloop_add_handler(server->ioloop,
                           listen_fd,
                           EPOLLIN,
                           _server_connection_handler,
                           server) < 0) {
        error("Error add connection handler");
        return -1;
    }
    return 0;
}

static int _configure_server(server_t *server, json_value *conf_obj) {
    json_value *val;
    char  *name, *port_str;
    site_conf_t  *site_conf = NULL;
    int i;
    int lvl = INFO;

    for (i = 0; i < conf_obj->u.object.length; i++) {
        name = conf_obj->u.object.values[i].name;
        val = conf_obj->u.object.values[i].value;
        if (strcmp("listen", name) == 0 && val->type == json_string) {
            port_str = strchr(val->u.string.ptr, ':');
            if (port_str != NULL) {
                *port_str = '\0';
                port_str++;
                server->port = (unsigned short) atoi(port_str);
            } else {
                server->port = 80;
            }
            server->addr = val->u.string.ptr;
        } else if(strcmp("sites", name) == 0) {
            site_conf = site_conf_parse(val);
            if (site_conf == NULL) {
                error("Error creating site conf");
                return -1;
            }
        } else if(strcmp("logfile", name) == 0 && val->type == json_string) {
            server->logfile = val->u.string.ptr;
        } else if(strcmp("daemonize", name) == 0 && val->type == json_boolean) {
            server->daemonize = val->u.boolean;
        } else if(strcmp("pidfile", name) == 0 && val->type == json_string) {
            server->pidfile = val->u.string.ptr;
        } else if(strcmp("loglevel", name) == 0 && val->type == json_string) {
            if (strcasecmp("debug", val->u.string.ptr) == 0) {
                lvl = DEBUG;
            } else if(strcasecmp("info", val->u.string.ptr) == 0) {
                lvl = INFO;
            } else if(strcasecmp("warn", val->u.string.ptr) == 0) {
                lvl = WARN;
            } else if(strcasecmp("error", val->u.string.ptr) == 0) {
                lvl = ERROR;
            } else {
                warn("Unknown log level: %s", val->u.string.ptr);
            }
            server->loglevel = lvl;
        } else {
            warn("Unknown config command %s with type %d", name, val->type);
        }

    }

    if (site_conf == NULL) {
        error("No site found");
        return -1;
    }
    server->handler = site_handler;
    server->handler_conf = site_conf;
    return 0;
}

static void _server_connection_handler(ioloop_t *loop,
                                       int listen_fd,
                                       unsigned int events,
                                       void *args)
{
    connection_t *conn;
    server_t     *server = (server_t*) args;

    while ((conn = connection_accept(server, listen_fd)) != NULL) {
        connection_run(conn);        
    }

}
