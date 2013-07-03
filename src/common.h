#ifndef __COMMON_H
#define __COMMON_H


enum _return_status {
    STATUS_ERROR = -1,
    STATUS_COMPLETE = 0,
    STATUS_CONTINUE = 1
};

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#endif /* end of include guard: __COMMON_H */
