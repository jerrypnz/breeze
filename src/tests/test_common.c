#include "common.h"
#include <assert.h>
#include <stdio.h>

void test_date_functions() {
    char buf[50];
    time_t time;
    
    assert(current_http_date(buf, 50) == 0);
    printf("Current time in HTTP format: %s\n", buf);

    assert(parse_http_date(buf, &time) == 0);
    printf("Current time in seconds: %ld\n", time);

    time += 81;
    
    format_http_date(&time, buf, 50);
    printf("Current time in HTTP format(add 81 secs): %s\n", buf);
}

void test_path_starts_with() {
    assert(path_starts_with("/", "/bar/11") > 0);
    assert(path_starts_with("/", "/") > 0);
    assert(path_starts_with("/bar", "/bar/11") > 0);
    assert(path_starts_with("/bar", "/bar/") > 0);
    assert(path_starts_with("/bar", "/bar") > 0);
    assert(path_starts_with("/bar", "/bar1") == 0);
    assert(path_starts_with("/bar/", "/bar/11") > 0);
}


int main(int argc, char *argv[]) {
    test_date_functions();
    test_path_starts_with();
    return 0;
}
