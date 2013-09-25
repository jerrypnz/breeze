#include "common.h"
#include "log.h"
#include "stacktrace.h"
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_TRACE_DEPTH 30

static void stacktrace_handler(int sig) {
    void    *trace[MAX_TRACE_DEPTH];
    size_t  depth;

    depth = backtrace(trace, MAX_TRACE_DEPTH);

    error("Error captured. Signal: %s", strsignal(sig));
    backtrace_symbols_fd(trace, depth, 2);
    exit(1);
}

void print_stacktrace_on_error() {
    signal(SIGSEGV, stacktrace_handler);
    signal(SIGFPE, stacktrace_handler);
    signal(SIGABRT, stacktrace_handler);
}
