#include "status.h"

// 1xx informational
http_status_t STATUS_CONTINUE = {100, "Continue"};

// 2xx success
http_status_t STATUS_OK = {200, "OK"};
http_status_t STATUS_CREATED = {201, "Created"};
http_status_t STATUS_ACCEPTED = {202, "Accepted"};
http_status_t STATUS_NO_CONTENT = {204, "No Content"};

// 3xx redirection
http_status_t STATUS_MOVED = {301, "Moved Permanently"};
http_status_t STATUS_FOUND = {302, "Found"};
http_status_t STATUS_SEE_OTHER = {303, "See Other"};
http_status_t STATUS_NOT_MODIFIED = {304, "Not Modified"};

// 4xx client errors
http_status_t STATUS_BAD_REQUEST = {400, "Bad Request"};
http_status_t STATUS_UNAUTHORIZED = {401, "Unauthorized"};
http_status_t STATUS_FORBIDDEN = {403, "Forbidden"};
http_status_t STATUS_NOT_FOUND = {404, "Not Found"};
http_status_t STATUS_METHOD_NOT_ALLOWED = {405, "Method Not Allowed"};

// 5xx server errors
http_status_t STATUS_INTERNAL_ERROR = {500, "Internal Server Error"};
http_status_t STATUS_NOT_IMPLEMENTED = {501, "Not Implemented"};
http_status_t STATUS_BAD_GATEWAY = {502, "Bad Gateway"};
http_status_t STATUS_SERVICE_UNAVAILABLE = {503, "Service Unavailable"};
http_status_t STATUS_GATEWAY_TIMEOUT = {504, "Gateway Timeout"};
