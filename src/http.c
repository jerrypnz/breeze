#include "http.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum _parser_state {
    PARSER_STATE_BAD_REQUEST = -1,
    PARSER_STATE_COMPLETE = 0,
    PARSER_STATE_METHOD,
    PARSER_STATE_PATH,
    PARSER_STATE_QUERY_STR,
    PARSER_STATE_VERSION,
    PARSER_STATE_HEADER_NAME,
    PARSER_STATE_HEADER_COLON,
    PARSER_STATE_HEADER_VALUE,
    PARSER_STATE_HEADER_CR,
    PARSER_STATE_HEADER_LF,
    PARSER_STATE_HEADER_COMPLETE_CR,
} parser_state_e;


static http_version_e _resolve_http_version(const char* version_str);


request_t* request_create() {
    request_t  *req;
    req = (request_t*) calloc(1, sizeof(request_t));
    if (req == NULL) {
        fprintf(stderr, "Unable to malloc");
        return NULL;
    }
    bzero(req, sizeof(request_t));
    if (hcreate_r(MAX_HEADER_SIZE, &req->_header_hash) == 0) {
        perror("Error creating header hash table");
        free(req);
        return NULL;
    }
    return req;
}

int request_destroy(request_t *req) {
    hdestroy_r(&req->_header_hash);
    free(req);
    return 0;
}

#define START_NEW_TOKEN(tok, req)                      \
    (tok = (req)->_buffer_in + (req)->_buf_in_idx)

#define FILL_NEXT_CHAR(req, ch) \
    ((req)->_buffer_in[req->_buf_in_idx++] = (ch))
    
#define FINISH_CUR_TOKEN(req) \
    ((req)->_buffer_in[(req)->_buf_in_idx++] = '\0' )

#define EXPECT_CHAR(state, ch, expected_ch, next_state) \
    if ((ch) == (expected_ch)) { \
        state = (next_state); \
    } else { \
        state = PARSER_STATE_BAD_REQUEST; \
    }


int request_parse_headers(request_t *req,
                          const char *data,
                          const size_t data_len,
                          size_t *consumed) {
    printf("Parsing HTTP request\n");
    http_version_e      ver;
    int                 i, rc;
    char                ch;
    char                *cur_token = req->_buffer_in;
    parser_state_e      state = PARSER_STATE_METHOD;

    req->_buffer_in[0] = '\0';
    req->_buf_in_idx = 0;
    req->header_count = 0;

    for (i = 0; i < data_len;){

        if (state == PARSER_STATE_COMPLETE ||
             state == PARSER_STATE_BAD_REQUEST) {
            break;
        } 
        ch = data[i++];

        switch(state) {

            case PARSER_STATE_METHOD:
                if (ch == ' ') {
                    FINISH_CUR_TOKEN(req);
                    req->method = cur_token;
                    state = PARSER_STATE_PATH;
                    START_NEW_TOKEN(cur_token, req);
                } else if (ch < 'A' || ch > 'Z') {
                    state = PARSER_STATE_BAD_REQUEST;
                } else {
                    FILL_NEXT_CHAR(req, ch);
                }
                break;

            case PARSER_STATE_PATH:
                if (ch == '?') {
                    FINISH_CUR_TOKEN(req);
                    req->path = cur_token;
                    state = PARSER_STATE_QUERY_STR;
                    START_NEW_TOKEN(cur_token, req);
                } else if (ch == ' ') {
                    FINISH_CUR_TOKEN(req);
                    req->path = cur_token;
                    state = PARSER_STATE_VERSION;
                    START_NEW_TOKEN(cur_token, req);
                } else {
                    FILL_NEXT_CHAR(req, ch);
                }
                break;

            case PARSER_STATE_QUERY_STR:
                if (ch == ' ') {
                    FINISH_CUR_TOKEN(req);
                    req->query_str = cur_token;
                    state = PARSER_STATE_VERSION;
                    START_NEW_TOKEN(cur_token, req);
                } else {
                    FILL_NEXT_CHAR(req, ch);
                }
                break;

            case PARSER_STATE_VERSION:
                switch (ch) {
                    // For HTTP part in the request line, e.g. GET / HTTP/1.1
                    case 'H':
                    case 'T':
                    case 'P':

                    // Currently only 0.9, 1.0 and 1.1 are supported.
                    case '0':
                    case '1':
                    case '9':
                    case '.':
                        FILL_NEXT_CHAR(req, ch);
                        break;

                    case '/':
                        FINISH_CUR_TOKEN(req);
                        if (strcmp("HTTP", cur_token) != 0) {
                            state = PARSER_STATE_BAD_REQUEST;
                            break;
                        }
                        START_NEW_TOKEN(cur_token, req);
                        break;

                    case '\r':
                        FINISH_CUR_TOKEN(req);
                        ver = _resolve_http_version(cur_token);
                        if (ver == HTTP_VERSION_UNKNOW) {
                            state = PARSER_STATE_BAD_REQUEST;
                            break;
                        }
                        req->version = ver;
                        state = PARSER_STATE_HEADER_CR;
                        START_NEW_TOKEN(cur_token, req);
                        break;
                }
                break;

            case PARSER_STATE_HEADER_NAME:
                if (ch == ':') {
                    FINISH_CUR_TOKEN(req);
                    req->headers[req->header_count].name = cur_token;
                    state = PARSER_STATE_HEADER_COLON;
                    START_NEW_TOKEN(cur_token, req);
                } else {
                    FILL_NEXT_CHAR(req, ch);
                }
                break;

            case PARSER_STATE_HEADER_COLON:
                EXPECT_CHAR(state, ch, ' ', PARSER_STATE_HEADER_VALUE);
                break;

            case PARSER_STATE_HEADER_VALUE:
                if (ch == '\r') {
                    FINISH_CUR_TOKEN(req);
                    req->headers[req->header_count].value = cur_token;
                    req->header_count++;
                    state = PARSER_STATE_HEADER_CR;
                    START_NEW_TOKEN(cur_token, req);
                } else {
                    FILL_NEXT_CHAR(req, ch);
                }
                break;

            case PARSER_STATE_HEADER_CR:
                EXPECT_CHAR(state, ch, '\n', PARSER_STATE_HEADER_LF);
                break;

            case PARSER_STATE_HEADER_LF:
                // Another CR after a header LF, meanning the header end is met.
                if (ch == '\r') { 
                    state = PARSER_STATE_HEADER_COMPLETE_CR; 
                } else { 
                    state = PARSER_STATE_HEADER_NAME; 
                    FILL_NEXT_CHAR(req, ch);
                }
                break;

            case PARSER_STATE_HEADER_COMPLETE_CR:
                EXPECT_CHAR(state, ch, '\n', PARSER_STATE_COMPLETE);
                break;

            default:
                fprintf(stderr, "Unexpected state: %d\n", state);
                break;

        }
    }

    *consumed = i;
    
    switch (state) {
        case PARSER_STATE_COMPLETE:
            rc = STATUS_COMPLETE;
            break;
            
        case PARSER_STATE_BAD_REQUEST:
            rc = STATUS_ERROR;
            break;

        default:
            rc = STATUS_CONTINUE;
            break;
    }

    return rc;
}

static http_version_e _resolve_http_version(const char* version_str) {
    if (strcmp(version_str, "1.1") == 0) {
        return HTTP_VERSION_1_1;
    } else if (strcmp(version_str, "1.0") == 0) {
        return HTTP_VERSION_1_0;
    } else if (strcmp(version_str, "0.9") == 0) {
        return HTTP_VERSION_0_9;
    }

    return HTTP_VERSION_UNKNOW;
}
