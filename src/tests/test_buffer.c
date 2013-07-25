#include "buffer.h"
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

void assert_equals(char *expected, char *actual, size_t sz) {
    int i;
    for (i = 0; i < sz; i++) {
        assert(expected[i] == actual[i]);
    }
}

buffer_t *create_buffer(size_t sz) {
    buffer_t    *buf = NULL;

    buf = buffer_create(sz);
    assert(buf != NULL);
    return buf;
}

void test_basic_case() {
    char        data[] = "1234567890";
    char        result[12];
    buffer_t    *buf = create_buffer(10);

    assert(buffer_write(buf, data, strlen(data)) == 0);
    assert(buffer_read(buf, 10, result, 12) == 10);
    assert_equals(data, result, 10);
    assert(buffer_destroy(buf) == 0);
}

void test_overflow() {
    char        data[] = "1234567890";
    buffer_t    *buf = create_buffer(5);

    assert(buffer_write(buf, data, strlen(data)) < 0);
    assert(buffer_destroy(buf) == 0);
}

void test_multiple_readwrite() {
    char        data1[] = "123456";
    char        data2[] = "abcd";
    char        data3[] = "mnpqrs";
    char        data4[] = "!@#$%^&*=";
    char        result[12];
    buffer_t    *buf = create_buffer(10);

    assert(buffer_write(buf, data1, strlen(data1)) == 0);
    assert(buffer_write(buf, data2, strlen(data2)) == 0);
    assert(buffer_read(buf, 8, result, 12) == 8);
    assert_equals("123456ab", result, 8);

    assert(buffer_write(buf, data3, strlen(data3)) == 0);
    assert(buffer_read(buf, 10, result, 12) == 8);
    assert_equals("cdmnpqrs", result, 8);

    assert(buffer_write(buf, data4, strlen(data4)) == 0);
    assert(buffer_read(buf, 10, result, 12) == 9);
    assert_equals(data4, result, 9);
    assert(buffer_destroy(buf) == 0);
}


void test_read_to_fd() {
    char        data[] = "1234567890abcdefg!@#$%^&*";
    buffer_t    *buf = create_buffer(30);
    int         fd;

    assert(buffer_write(buf, data, strlen(data)) == 0);
    fd = open("/tmp/foobar.txt", O_RDWR | O_CREAT, 0755);
    assert(buffer_flush(buf, fd) == strlen(data));
    fsync(fd);
    close(fd);
    assert(buffer_destroy(buf) == 0);
}

void test_write_from_fd() {
    char        data[] = "1234567890abcdefg!@#$%^&*";
    char        result[100];
    buffer_t    *buf = create_buffer(512);
    int         fd;
    size_t      data_len;

    data_len = strlen(data);
    fd = open("/tmp/foobar2.txt", O_RDWR | O_CREAT, 0755);
    assert(write(fd, data, data_len) > 0);
    lseek(fd, 0, SEEK_SET);
    assert(buffer_fill(buf, fd) == data_len);
    assert(buffer_read(buf, data_len, result, 100) == data_len);
    assert_equals(data, result, data_len);
}

void test_write_from_fd_overflow() {
    char        data[] = "1234567890abcdefg!@#$%^&*";
    char        result[100];
    buffer_t    *buf = create_buffer(10);
    int         fd;
    size_t      data_len;

    data_len = strlen(data);
    fd = open("/tmp/foobar3.txt", O_RDWR | O_CREAT, 0755);
    assert(write(fd, data, data_len) > 0);
    lseek(fd, 0, SEEK_SET);
    assert(buffer_fill(buf, fd) == 10);
    assert(buffer_is_full(buf));
    assert(buffer_read(buf, 10, result, 100) == 10);
    assert_equals(data, result, 10);
    assert(buffer_destroy(buf) == 0);
}

#define SECRET_ARG "foobar"

static void _print_consumer(void *data, size_t len, void *args) {
    char *str = (char*) data;
    int i;

    assert_equals((char*) args, SECRET_ARG, 6);
    for (i = 0; i < len; i++) {
        putchar(str[i]);
    }
    putchar('\n');
}


void test_consume() {
    char        data[] = "12345";
    char        data1[] = "abcdefghij";
    buffer_t    *buf = create_buffer(10);

    assert(buffer_write(buf, data, strlen(data)) == 0);
    assert(buffer_consume(buf, 10, _print_consumer, SECRET_ARG) == strlen(data));

    assert(buffer_write(buf, data1, strlen(data1)) == 0);
    assert(buffer_consume(buf, 10, _print_consumer, SECRET_ARG) == strlen(data1));

    assert(buffer_write(buf, data, strlen(data)) == 0);
    assert(buffer_consume(buf, 2, _print_consumer, SECRET_ARG) == 2);
    assert(buffer_consume(buf, 3, _print_consumer, SECRET_ARG) == 3);
    assert(buffer_destroy(buf) == 0);
}

void test_locate() {
    char        data[] = "abcde";
    char        data1[] = "0123456789";
    buffer_t    *buf = create_buffer(10);

    assert(buffer_write(buf, data, strlen(data)) == 0);
    assert(buffer_locate(buf, "cd") == 2);
    assert(buffer_locate(buf, "c") == 2);
    assert(buffer_locate(buf, "d") == 3);
    assert(buffer_locate(buf, "a") == 0);
    assert(buffer_locate(buf, "e") == 4);
    assert(buffer_locate(buf, "kkk") < 0);

    assert(buffer_consume(buf, 10, _print_consumer, SECRET_ARG) == strlen(data));

    assert(buffer_write(buf, data1, strlen(data1)) == 0);
    assert(buffer_locate(buf, "012") == 0);
    assert(buffer_locate(buf, "0") == 0);
    assert(buffer_locate(buf, "9") == 9);
    assert(buffer_locate(buf, "6") == 6);
    assert(buffer_locate(buf, "3456") == 3);
    assert(buffer_locate(buf, "89") == 8);
    assert(buffer_locate(buf, "kkk") < 0);
    assert(buffer_destroy(buf) == 0);
}

int main(int argc, char *argv[]) {
    test_basic_case();
    test_overflow();
    test_multiple_readwrite();
    test_read_to_fd();
    test_write_from_fd();
    test_write_from_fd_overflow();
    test_consume();
    test_locate();
    return 0;
}
