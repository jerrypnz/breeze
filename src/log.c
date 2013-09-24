#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

static const char* LEVEL_NAMES[] = {"DEBUG", "INFO", "WARN", "ERROR"};

static int enable_console = 1;
static FILE *log_stream = NULL;
static int log_level = INFO;

int configure_log(int lvl, const char* file, int use_console) {
    FILE *stream = NULL;
    int res = 0;

    if (lvl > ERROR)
        lvl = ERROR;
    else if (lvl < DEBUG)
        lvl = DEBUG;
    log_level = lvl;
    enable_console = use_console;
    if (file != NULL) {
        stream = fopen(file, "a");
        if (stream == NULL) {
            perror("Error opening log file");
            res = 1;
        } else {
            log_stream = stream;
        }
    }

    return res;
}

void logging(int lvl, const char *file, const int line, const char *fmt, ...) {
    va_list ap;
    char buffer[512], *ptr = buffer;
    int size, cap = 512;
    time_t ts;
    struct tm *tmp;

    if (lvl < log_level) {
        return;
    }

    ts = time(NULL);
    tmp = localtime(&ts);
    size = strftime(ptr, cap, "[%Y-%m-%d %H:%M:%S]", tmp);
    ptr += size;
    cap -= size;
    size = snprintf(ptr, cap, "[%-5s][%s:%d] ",
                    LEVEL_NAMES[lvl], file, line);
    ptr += size;
    cap -= size;

    va_start(ap, fmt);
    size = vsnprintf(ptr, cap, fmt, ap);
    va_end(ap);

    *(ptr + size) = '\n';
    *(ptr + size + 1) = '\0';

    if (enable_console) {
        if (lvl >= WARN) {
            fputs(buffer, stderr);
        } else {
            puts(buffer);
        }
    }

    if (log_stream != NULL) {
        fputs(buffer, log_stream);
    }
    
}
