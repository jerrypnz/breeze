#ifndef __CONNECTION_H_
#define __CONNECTION_H_

typedef struct _connection connection_t;

connection_t*     connection_create(iostream_t *stream);
int               connection_close(connection_t *conn);
int               connection_destroy(connection_t *conn);

#endif /* __CONNECTION_H_ */
