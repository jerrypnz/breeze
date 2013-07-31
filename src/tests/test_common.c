#include "common.h"
#include <assert.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    char buf[50];
    assert(current_http_date(buf, 50) == 0);
    printf("Current time in HTTP format: %s\n", buf);
    return 0;
}
