CC = gcc
CFLAGS ?= -O2 -g -rdynamic -Wall -I.
LDFLAGS ?= -O2 -g -rdynamic -lcrypt

objects = common.o ioloop.o buffer.o iostream.o http.o server.o location.o stacktrace.o http_connection.o http_server.o mod_static.o
testobjs = test_common.o test_buffer.o test_ioloop.o test_iostream.o test_http.o test_http_server.o
executables = test_common test_buffer test_ioloop test_iostream test_http test_http_server mod_static

vpath %.c tests

.PHONY: all
all: $(executables)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

test_%: test_%.o %.o common.o
	$(CC) $(LDFLAGS) $^ -o $@

test_iostream: ioloop.o buffer.o
test_http: stacktrace.o iostream.o ioloop.o buffer.o http_connection.o
test_http_server: http_connection.o iostream.o ioloop.o buffer.o http.o

mod_static: common.o http_connection.o iostream.o ioloop.o buffer.o http.o http_server.o mod_static.o
	$(CC) $(LDFLAGS) $^ -o $@	

.PHONY: clean
clean:
	-rm -f $(objects) $(executables) $(tests) $(testobjs)
	@echo Project cleaned

