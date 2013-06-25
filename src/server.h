#ifndef __SERVER_H_
#define __SERVER_H_

struct _server;

typedef struct _server server_t;

server_t*       server_create(unsigned short port, char *confile);
int             server_destroy(server_t *server);
int             server_start(server_t *server);
int             server_stop(server_t *server);

#endif /* __SERVER_H_ */
