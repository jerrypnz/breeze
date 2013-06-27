CC = gcc
CFLAGS ?= -g -rdynamic -Wall -I.
LDFLAGS ?= -g -rdynamic

objects = ioloop.o buffer.o iostream.o http.o server.o location.o stacktrace.o
testobjs = test_buffer.o test_ioloop.o test_iostream.o test_http.o
executables = test_buffer test_ioloop test_iostream test_http

vpath %.c tests

.PHONY: all
all: $(executables)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

test_%: test_%.o %.o
	$(CC) $(LDFLAGS) $^ -o $@

test_iostream: ioloop.o buffer.o
test_http: stacktrace.o

.PHONY: clean
clean:
	-rm -f $(objects) $(executables) $(tests) $(testobjs)
	@echo Project cleaned

