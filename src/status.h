#ifndef __STATUS_H_
#define __STATUS_H_

#include "http.h"

// 1xx informational
extern http_status_t STATUS_CONTINUE;

// 2xx success
extern http_status_t STATUS_OK;
extern http_status_t STATUS_CREATED;
extern http_status_t STATUS_ACCEPTED;
extern http_status_t STATUS_NO_CONTENT;

// 3xx redirection
extern http_status_t STATUS_MOVED;
extern http_status_t STATUS_FOUND;
extern http_status_t STATUS_SEE_OTHER;
extern http_status_t STATUS_NOT_MODIFIED;

// 4xx client errors
extern http_status_t STATUS_BAD_REQUEST;
extern http_status_t STATUS_UNAUTHORIZED;
extern http_status_t STATUS_FORBIDDEN;
extern http_status_t STATUS_NOT_FOUND;
extern http_status_t STATUS_METHOD_NOT_ALLOWED;

// 5xx server errors
extern http_status_t STATUS_INTERNAL_ERROR;
extern http_status_t STATUS_NOT_IMPLEMENTED;
extern http_status_t STATUS_BAD_GATEWAY;
extern http_status_t STATUS_SERVICE_UNAVAILABLE;
extern http_status_t STATUS_GATEWAY_TIMEOUT;

#endif /* __STATUS_H_ */
