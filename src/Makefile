CC = gcc
CFLAGS ?= -g -rdynamic -Wall -I.
LDFLAGS ?= -g -rdynamic

objects = ioloop.o buffer.o iostream.o
testobjs = test_buffer.o test_ioloop.o test_iostream.o
executables = test_buffer test_ioloop test_iostream

vpath %.c ../tests

.PHONY: all
all: $(executables)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

test_%: test_%.o %.o
	$(CC) $(LDFLAGS) $^ -o $@

test_iostream: ioloop.o buffer.o

.PHONY: clean
clean:
	-rm -f $(objects) $(executables) $(tests) $(testobjs)
	@echo Project cleaned

