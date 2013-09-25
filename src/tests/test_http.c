#include "http.h"
#include "common.h"
#include "log.h"
#include "stacktrace.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>


char* test_request = 
    "GET /home/hello.do?id=1001&name=hello HTTP/1.1\r\n"
    "Host: www.javaeye.com\r\n"
    "Connection: keep-alive\r\n"
    "Referer: http://www.javaeye.com/\r\n"
    "Cache-Control: max-age=0\r\n"
    "If-Modified-Since: Sat, 15 Dec 2007 04:04:14 GMT\r\n"
    "If-None-Match: \"3922745954\"\r\n"
    "Accept: */*\r\n"
    "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US) AppleWebKit/534.15 (KHTML, like Gecko) Chrome/10.0.612.1 Safari/534.15\r\n"
    "Accept-Encoding: gzip,deflate,sdch\r\n"
    "Accept-Language: zh-CN,zh;q=0.8\r\n"
    "Accept-Charset: GBK,utf-8;q=0.7,*;q=0.3\r\n"
    "Cookie: lzstat_uv=39255935923757216993|1146165@1270080@1632275@1145010@0; remember_me=yes; login_token=MTAyMjExXzJkZjY4ZGIyNDIzZTdjMTE4YmM5OTU0OTQ2MTU0N2Fh%0A; _javaeye3_session_=BAh7BzoMdXNlcl9pZGkDQ48BOg9zZXNzaW9uX2lkIiVjMDdlNjJmZGJiNTFhZjhhYmU5Yjk4NjE1Y2ZjODBhOQ%3D%3D--b9945c8e50a5eba75e75c92b2c92e5a3a86f1000\r\n\r\n";

char* test_request_2 = 
    "GET /home/hello.do?id=1001&name=hello HTTP/1.1\r\n"
    "Host: www.javaeye.com\r\n"
    "Connection: keep-alive\r\n"
    "Referer: http://www.javaeye.com/\r\n"
    "Cache-Control: max-age=0\r\n"
    "If-Modified-Since: Sat, 15 Dec 2007 04:04:14 GMT\r\n"
    "If-None-Match: \"3922745954\"\r\n"
    "Accept: */*\r\n"
    "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US) AppleWebKit/534.15 (KHTML, like Gecko) Chrome/10.0.612.1 Safari/534.15\r\n"
    "Accept-Encoding: gzip,deflate,sdch\r\n"
    "Accept-Language: zh-CN,zh;q=0.8\r\n"
    "Accept-Charset: GBK,utf-8;q=0.7,*;q=0.3\r\n"
    "Cookie: lzstat_uv=39255935923757216993|1146165@1270080@1632275@1145010@0; remember_me=yes; login_token=MTAyMjExXzJkZjY4ZGIyNDIzZTdjMTE4YmM5OTU0OTQ2MTU0N2Fh%0A; _javaeye3_session_=BAh7BzoMdXNlcl9pZGkDQ48BOg9zZXNzaW9uX2lkIiVjMDdlNjJmZGJiNTFhZjhhYmU5Yjk4NjE1Y2ZjODBhOQ%3D%3D--b9945c8e50a5eba75e75c92b2c92e5a3a86f1000\r\n\r\n"
    "GET /home/hello.do?id=1001&name=hello HTTP/1.1\r\n"; // This line is the start of another request

char* invalid_req = 
    "GET /home/hello.do?id=1001&name=hello HTTP/1.9\r\n"
    "Host: www.javaeye.com\r\n"
    "Connection: keep-alive\r\n"
    "Referer: http://www.javaeye.com/\r\n"
    "Cache-Control: max-age=0\r\n"
    "If-Modified-Since: Sat, 15 Dec 2007 04:04:14 GMT\r\n"
    "If-None-Match: \"3922745954\"\r\n"
    "Accept: */*\r\n"
    "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US) AppleWebKit/534.15 (KHTML, like Gecko) Chrome/10.0.612.1 Safari/534.15\r\n"
    "Accept-Encoding: gzip,deflate,sdch\r\n"
    "Accept-Language: zh-CN,zh;q=0.8\r\n"
    "Accept-Charset: GBK,utf-8;q=0.7,*;q=0.3\r\n"
    "Cookie: lzstat_uv=39255935923757216993|1146165@1270080@1632275@1145010@0; remember_me=yes; login_token=MTAyMjExXzJkZjY4ZGIyNDIzZTdjMTE4YmM5OTU0OTQ2MTU0N2Fh%0A; _javaeye3_session_=BAh7BzoMdXNlcl9pZGkDQ48BOg9zZXNzaW9uX2lkIiVjMDdlNjJmZGJiNTFhZjhhYmU5Yjk4NjE1Y2ZjODBhOQ%3D%3D--b9945c8e50a5eba75e75c92b2c92e5a3a86f1000\r\n\r\n";

const http_header_t     expected_headers[] = {
    {"Host", "www.javaeye.com"},
    {"Connection", "keep-alive"},
    {"Referer", "http://www.javaeye.com/"},
    {"Cache-Control", "max-age=0"},
    {"If-Modified-Since", "Sat, 15 Dec 2007 04:04:14 GMT"},
    {"If-None-Match", "\"3922745954\""},
    {"Accept", "*/*"},
    {"User-Agent", "Mozilla/5.0 (X11; U; Linux i686; en-US) AppleWebKit/534.15 (KHTML, like Gecko) Chrome/10.0.612.1 Safari/534.15"},
    {"Accept-Encoding", "gzip,deflate,sdch"},
    {"Accept-Language", "zh-CN,zh;q=0.8"},
    {"Accept-Charset", "GBK,utf-8;q=0.7,*;q=0.3"},
    {"Cookie", "lzstat_uv=39255935923757216993|1146165@1270080@1632275@1145010@0; remember_me=yes; login_token=MTAyMjExXzJkZjY4ZGIyNDIzZTdjMTE4YmM5OTU0OTQ2MTU0N2Fh%0A; _javaeye3_session_=BAh7BzoMdXNlcl9pZGkDQ48BOg9zZXNzaW9uX2lkIiVjMDdlNjJmZGJiNTFhZjhhYmU5Yjk4NjE1Y2ZjODBhOQ%3D%3D--b9945c8e50a5eba75e75c92b2c92e5a3a86f1000"}
};

void dump_request(request_t *req);
void assert_headers(request_t *req);

static void assert_equals(const char* expected, const char *actual) {
    assert(strcmp(expected, actual) == 0);
}

void test_parse_once() {
    request_t  *req;
    size_t     req_size, consumed_size;
    int        rc;

    req = request_create(NULL);
    assert(req != NULL);
    info("\n\nTesting parsing all in one time");
    req_size = strlen(test_request);
    rc = request_parse_headers(req, test_request, req_size, &consumed_size);
    info("Request size: %zu, consumed size: %zu, return status: %d",
         req_size, consumed_size, rc);
    dump_request(req);
    assert(rc == STATUS_COMPLETE);
    assert(consumed_size == req_size);
    assert_headers(req);
    assert(request_destroy(req) == 0);
}


void test_parse_once_with_extra_data() {
    request_t  *req;
    size_t     req_size, consumed_size;
    int        rc;    

    req = request_create(NULL);
    assert(req != NULL);
    info("\n\nTesting parsing all in one time with extra data left");
    req_size = strlen(test_request_2);
    rc = request_parse_headers(req, test_request_2, req_size, &consumed_size);
    info("Request size: %zu, consumed size: %zu, return status: %d", req_size, consumed_size, rc);
    dump_request(req);
    assert(rc == STATUS_COMPLETE);
    assert(consumed_size == req_size - strlen("GET /home/hello.do?id=1001&name=hello HTTP/1.1\r\n"));
    assert_headers(req);
    assert(request_destroy(req) == 0);
}

void test_parse_multiple_times() {
    request_t  *req;
    size_t     req_size, consumed_size;
    int        rc;
    size_t     part1_size;
    char       *data;

    req = request_create(NULL);
    assert(req != NULL);
    info("\n\nTesting parsing all in multiple time");
    data = test_request;
    req_size = strlen(data);
    part1_size = req_size / 2;

    // Incomplete request
    rc = request_parse_headers(req, data, part1_size, &consumed_size);
    info("First Time: Request size: %zu, consumed size: %zu, return status: %d",
         req_size, consumed_size, rc);
    dump_request(req);
    assert(rc == STATUS_INCOMPLETE);
    assert(consumed_size == part1_size);
    assert(request_destroy(req) == 0);
}


void test_parse_invalid_version() {
    request_t *req;
    int     req_size, rc;
    size_t  consumed_size;

    req = request_create(NULL);
    assert(req != NULL);
    req_size = strlen(invalid_req);

    info("\n\nTesting invalid HTTP version");
    rc = request_parse_headers(req, invalid_req, req_size, &consumed_size);
    dump_request(req);
    assert(rc == STATUS_ERROR);
    assert(request_destroy(req) == 0);
}


char* test_request_3 = 
    "GET /home/hello.do?id=1001&name=hello HTTP/1.1\r\n"
    "Host: www.javaeye.com\r\n"
    "Connection: keep-alive\r\n"
    "Content-Length: 42\r\n"
    "Referer: http://www.javaeye.com/\r\n\r\n";

void test_common_header_handling() {
    request_t *req;
    int     req_size, rc;
    size_t  consumed_size;

    req = request_create(NULL);
    assert(req != NULL);
    req_size = strlen(test_request_3);

    info("\n\nTesting common header handling");
    rc = request_parse_headers(req, test_request_3, req_size, &consumed_size);
    dump_request(req);
    assert(rc == STATUS_COMPLETE);
    assert(strcmp(req->host, "www.javaeye.com") == 0);
    assert(req->content_length == 42);
    assert(req->connection == CONN_KEEP_ALIVE);
    assert(request_destroy(req) == 0);    
}


void dump_request(request_t *req) {
    int i;

    info("--------- Parser State -----------------------");
    info("Method: %s", req->method);
    info("Path: %s", req->path);
    info("Query String: %s", req->query_str);
    info("HTTP Version: %d", req->version);
    info("Header count: %zu", req->header_count);
    info("Headers: ");
    info("------------");

    for (i = 0; i < req->header_count; i++) {
        info("\r%s: %s", req->headers[i].name, req->headers[i].value);
    }

    info("----------------------------------------------");
}

void assert_headers(request_t *req) {
    int expected_header_size, i;
    const char* value;

    expected_header_size = sizeof(expected_headers) / sizeof(http_header_t);

    info("Expecting %d headers", expected_header_size);
    assert(expected_header_size == req->header_count);
    info("Checked");

    for (i = 0; i < req->header_count; i++) {
        assert(strcmp(expected_headers[i].name,  req->headers[i].name) == 0);
        assert(strcmp(expected_headers[i].value, req->headers[i].value) == 0);
        value = request_get_header(req, expected_headers[i].name);
        assert(value != NULL);
        assert(strcmp(expected_headers[i].value, value) == 0);
    }
}

void dump_response(response_t *resp) {
    int i;

    info("--------- Response State -----------------------");
    info("Header count: %zu", resp->header_count);
    info("Headers: ");
    info("------------");

    for (i = 0; i < resp->header_count; i++) {
        info("\r%s: %s", resp->headers[i].name, resp->headers[i].value);
    }

    info("----------------------------------------------");
}


void test_response_set_header_basic() {
    response_t *response;
    response = response_create(NULL);
    assert(response != NULL);

    // Basic tests
    assert(response_set_header(response, "Content-Length", "201") == 0);
    assert(response_set_header(response, "Content-Type", "application/html") == 0);

    info("After setting content-length and content-type");
    dump_response(response);

    assert(response->header_count == 2);
    assert_equals(response->headers[0].name, "Content-Length");
    assert_equals(response->headers[0].value, "201");

    assert_equals(response->headers[1].name, "Content-Type");
    assert_equals(response->headers[1].value, "application/html");

    assert_equals(response_get_header(response, "Content-Type"), "application/html");
    assert_equals(response_get_header(response, "Content-type"), "application/html");

    // Replace header value
    assert(response_set_header(response, "Content-Length", "1024") == 0);
    info("After setting content-length again");
    dump_response(response);

    assert_equals(response->headers[0].value, "1024");
    assert_equals(response_get_header(response, "content-length"), "1024");

    // Set unknow, non-standard headers
    assert(response_set_header(response, "X-Foobar", "foobar") == 0);
    info("After setting non-standard header");
    dump_response(response);

    assert_equals(response->headers[2].name, "X-Foobar");
    assert_equals(response->headers[2].value, "foobar");
    assert_equals(response_get_header(response, "x-Foobar"), "foobar");

    assert_equals(response->_buffer, "x-foobar");
    assert(response->_buf_idx == strlen("x-foobar") + 1);

    assert(response_destroy(response) == 0);
}

int main(int argc, const char *argv[])
{
    print_stacktrace_on_error();
    //bzero(&request, sizeof(request_t));
    test_parse_once();
    test_parse_once_with_extra_data();
    test_parse_multiple_times();
    test_parse_invalid_version();
    test_common_header_handling();
    test_response_set_header_basic();
    return 0;
}
