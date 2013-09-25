#include "ioloop.h"
#include "iostream.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>

static void connection_handler(ioloop_t *loop, int fd, unsigned int events, void *args);
static void connection_close_handler(iostream_t *stream);
static void read_bytes(iostream_t *stream, void* data, size_t len);
static void read_headers(iostream_t *stream, void *data, size_t len);
static void write_texts(iostream_t *stream);
static void send_file(iostream_t *stream);
static void close_stream(iostream_t *stream);
static void dump_data(void *data, size_t len);

/**
 * All Modes:
 * 0: Test read bytes
 * 1: Test read until
 * 2: Test write
 * 3: Test send file
 *
 */
static int mode = 0;
static char filename[1024];

static void connection_handler(ioloop_t *loop, int listen_fd, unsigned int events, void *args) {
    socklen_t   addr_len;
    int         conn_fd;
    struct sockaddr_in  remo_addr;
    iostream_t *stream;

    // -------- Accepting connection ----------------------------
    debug("Accepting new connection...");
    addr_len = sizeof(struct sockaddr_in);
    conn_fd = accept(listen_fd, (struct sockaddr*) &remo_addr, &addr_len);
    debug("Connection fd: %d...", conn_fd);
    if (conn_fd == -1) {
        error("Error accepting new connection");
        return;
    }

    if (set_nonblocking(conn_fd)) {
        error("Error configuring Non-blocking");
        return;
    }
    stream = iostream_create(loop, conn_fd, 1024, 1024, NULL);
    iostream_set_close_handler(stream, connection_close_handler);
    switch(mode) {
    case 0:
        error("Testing read 16 bytes");
        iostream_read_bytes(stream, 16, read_bytes, NULL);
        break;
            
    case 1:
        error("Testing read_until two blank lines(\\n\\n)");
        iostream_read_until(stream, "\r\n\r\n", read_headers);
        break;

    case 2:
        error("Testing writing dummy data");
        write_texts(stream);
        break;

    case 3:
        error("Testing sending file");
        send_file(stream);
        break;

    default:
        error("Unknown mode: read_until two blank lines(\\n)");
        iostream_read_until(stream, "\r\n\r\n", read_headers);
        break;
    }
}

static void read_bytes(iostream_t *stream, void* data, size_t len) {
    dump_data(data, len);
    iostream_read_bytes(stream, 16, read_bytes, NULL);
}

static void read_headers(iostream_t *stream, void *data, size_t len) {
    dump_data(data, len);
    iostream_read_until(stream, "\r\n\r\n", read_headers);
}

static void close_stream(iostream_t *stream) {
    debug("closing stream: %d", stream->fd);
    iostream_close(stream);
}

static void write_texts(iostream_t *stream) {
    static int counter = 0;
    char    buf[200];
    snprintf(buf, 200, "%d: 1234567890abcdefghijklmnopqrstuvwxyz-=_+{}()\r\n", counter++);
    if (counter == 1000) {
        counter = 0;
    }
    iostream_write(stream, buf, strlen(buf), close_stream);
}

static void send_file(iostream_t *stream) {
    int fd;
    struct stat st;
    size_t len;

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        error("Error opening file");
        return;
    }

    if (fstat(fd, &st) < 0) {
        error("Error get the st of the file");
        return;
    }

    len = st.st_size;

    if (len == 0) {
        error("File is empty");
        return;
    }

    iostream_sendfile(stream, fd, 0, len, close_stream);
}

static void dump_data(void *data, size_t len) {
    char    *str = (char*) data;
    int     i;

    info("Data read:\n------------------");
    for (i = 0; i < len; i++) {
        putchar(str[i]);
    }
    info("--------------------");
}


static void connection_close_handler(iostream_t *stream) {
    info("Closing connection.");
}

int main(int argc, char *argv[]) {
    ioloop_t               *loop;
    int                     listen_fd;
    struct sockaddr_in      addr;

    loop = ioloop_create(100);
    if (loop == NULL) {
        error("Error initializing ioloop");
        return -1;
    }

    // ---------- Create and bind listen socket fd --------------
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        error("Error creating socket");
        return -1;
    }

    bzero(&addr, sizeof(struct sockaddr_in));
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(9999);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
        error("Error binding address");
        return -1;
    }

    // ------------ Start listening ------------------------------
    if (listen(listen_fd, 10) == -1) {
        error("Error listening");
        return -1;
    }

    if (argc > 1) {
        mode = atoi(argv[1]);
        if (mode == 3) {
            if (argc < 3) 
                error("Please specify a file name");
            else
                strcpy(filename, argv[2]);
        }
    }

    ioloop_add_handler(loop, listen_fd, EPOLLIN, connection_handler, NULL);
    ioloop_start(loop);
    return 0;
}

