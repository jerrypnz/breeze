#ifndef __CONNECTION_H_
#define __CONNECTION_H_

#include <sys/socket.h>

typedef enum _connection_state {
    ACTIVE,
    CLOSING,
    CLOSED
} conn_stat_e;

typedef struct _connection {
    server_t           *server;
    iostream_t         *stream;
    struct sockaddr_in remote_addr;
    conn_stat_e        state;
} connection_t;

connection_t*     connection_create(server_t *server, iostream_t *stream);
int               connection_close(connection_t *conn);
int               connection_destroy(connection_t *conn);
int               connection_run(connection_t *conn);
int               connection_finish_current_request(connection_t *conn);

#endif /* __CONNECTION_H_ */
