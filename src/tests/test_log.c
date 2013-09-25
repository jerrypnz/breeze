#include "log.h"

int main(int argc, char *argv[])
{
    debug("This should not appear");
    info("This should appear in stdout");
    configure_log(WARN, "/tmp/test.log", 1);
    info("This should not appear now");
    warn("This should appear in stderr and log file");
    error("This should appear in stderr and log file");
    return 0;
}
