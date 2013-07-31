#include "common.h"
#include <assert.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    char buf[50];
    time_t time;
    
    assert(current_http_date(buf, 50) == 0);
    printf("Current time in HTTP format: %s\n", buf);

    assert(parse_http_date(buf, &time) == 0);
    printf("Current time in seconds: %ld\n", time);

    time += 81;
    
    format_http_date(&time, buf, 50);
    printf("Current time in HTTP format(add 81 secs): %s\n", buf);
    
    return 0;
}
