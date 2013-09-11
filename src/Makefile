CC = gcc
CFLAGS ?= -g -rdynamic -Wall -I. -I./json
LDFLAGS ?= -g -rdynamic -lcrypt -lm

objects = common.o ioloop.o buffer.o iostream.o http.o server.o stacktrace.o http_connection.o http_server.o site.o json.o mod_static.o conf_test.o
testobjs = test_common.o test_buffer.o test_ioloop.o test_iostream.o test_http.o test_http_server.o test_site.o
executables = test_common test_buffer test_ioloop test_iostream test_http test_http_server mod_static test_site conf_test

vpath %.c tests json

.PHONY: all
all: $(executables)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

test_%: test_%.o %.o common.o json.o stacktrace.o
	$(CC) $(LDFLAGS) $^ -o $@

test_iostream: ioloop.o buffer.o
test_site: http.o ioloop.o iostream.o buffer.o http_connection.o
test_http: stacktrace.o iostream.o ioloop.o buffer.o http_connection.o
test_http_server: http_connection.o iostream.o ioloop.o buffer.o http.o site.o
test_conf: http_connection.o iostream.o ioloop.o buffer.o http.o site.o

mod_static: common.o json.o http_connection.o iostream.o ioloop.o buffer.o http.o site.o http_server.o mod_static.o
	$(CC) $(LDFLAGS) $^ -o $@	

conf_test: common.o json.o http_connection.o iostream.o ioloop.o buffer.o http.o site.o http_server.o conf_test.o site.o stacktrace.o
	$(CC) $(LDFLAGS) $^ -o $@	

.PHONY: clean
clean:
	-rm -f $(objects) $(executables) $(tests) $(testobjs)
	@echo Project cleaned

