#include "http.h"
#include "common.h"
#include <stdio.h>
#include <string.h>

static http_version_e _resolve_http_version(const char* version_str);

int parser_init(http_parser_t *parser) {
    printf("Init parser init\n");
    parser_reset(parser);
    return 0;
}

int parser_destroy(http_parser_t *parser) {
    printf("In parser destroy\n");
    return 0;
}

int parser_reset(http_parser_t *parser) {
    printf("Init parser reset\n");

    // The first part of the request must be HTTP method.
    parser->_state = PARSER_STATE_METHOD;

    // Make the buffer an empty string
    parser->req->_buffer_in[0] = '\0';
    parser->req->_buf_in_idx = 0;
    parser->req->raw_header_count = 0;
    parser->_cur_tok = parser->req->_buffer_in;
    return 0;
}


#define START_NEW_TOKEN(parser) \
    ((parser)->_cur_tok = (parser)->req->_buffer_in + (parser)->req->_buf_in_idx)

#define FILL_NEXT_CHAR(parser, ch) \
    ((parser)->req->_buffer_in[(parser)->req->_buf_in_idx++] = (ch))
    
#define FINISH_CUR_TOKEN(parser) \
    ((parser)->req->_buffer_in[(parser)->req->_buf_in_idx++] = '\0' )

#define EXPECT_CHAR(parser, ch, expected_ch, next_state) \
    if ((ch) == (expected_ch)) { \
        (parser)->_state = (next_state); \
    } else { \
        (parser)->_state = PARSER_STATE_BAD_REQUEST; \
    }


int parse_request(http_parser_t *parser,
                  const char *data,
                  const size_t data_len,
                  size_t *consumed_len) {
    printf("Parsing HTTP request\n");
    http_version_e      ver;
    int                 i, rc;
    char                ch;

    for (i = 0; i < data_len;){

        if (parser->_state == PARSER_STATE_COMPLETE ||
             parser->_state == PARSER_STATE_BAD_REQUEST) {
            break;
        } 
        ch = data[i++];

        switch(parser->_state) {

            case PARSER_STATE_METHOD:
                if (ch == ' ') {
                    FINISH_CUR_TOKEN(parser);
                    parser->req->method = parser->_cur_tok;
                    parser->_state = PARSER_STATE_PATH;
                    START_NEW_TOKEN(parser);
                } else if (ch < 'A' || ch > 'Z') {
                    parser->_state = PARSER_STATE_BAD_REQUEST;
                } else {
                    FILL_NEXT_CHAR(parser, ch);
                }
                break;

            case PARSER_STATE_PATH:
                if (ch == '?') {
                    FINISH_CUR_TOKEN(parser);
                    parser->req->path = parser->_cur_tok;
                    parser->_state = PARSER_STATE_QUERY_STR;
                    START_NEW_TOKEN(parser);
                } else if (ch == ' ') {
                    FINISH_CUR_TOKEN(parser);
                    parser->req->path = parser->_cur_tok;
                    parser->_state = PARSER_STATE_VERSION;
                    START_NEW_TOKEN(parser);
                } else {
                    FILL_NEXT_CHAR(parser, ch);
                }
                break;

            case PARSER_STATE_QUERY_STR:
                if (ch == ' ') {
                    FINISH_CUR_TOKEN(parser);
                    parser->req->query_str = parser->_cur_tok;
                    parser->_state = PARSER_STATE_VERSION;
                    START_NEW_TOKEN(parser);
                } else {
                    FILL_NEXT_CHAR(parser, ch);
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
                        FILL_NEXT_CHAR(parser, ch);
                        break;

                    case '/':
                        FINISH_CUR_TOKEN(parser);
                        if (strcmp("HTTP", parser->_cur_tok) != 0) {
                            parser->_state = PARSER_STATE_BAD_REQUEST;
                            break;
                        }
                        START_NEW_TOKEN(parser);
                        break;

                    case '\r':
                        FINISH_CUR_TOKEN(parser);
                        ver = _resolve_http_version(parser->_cur_tok);
                        if (ver == HTTP_VERSION_UNKNOW) {
                            parser->_state = PARSER_STATE_BAD_REQUEST;
                            break;
                        }
                        parser->req->version = ver;
                        parser->_state = PARSER_STATE_HEADER_CR;
                        START_NEW_TOKEN(parser);
                        break;
                }
                break;

            case PARSER_STATE_HEADER_NAME:
                if (ch == ':') {
                    FINISH_CUR_TOKEN(parser);
                    parser->req->raw_headers_in[parser->req->raw_header_count].name
                        = parser->_cur_tok;
                    parser->_state = PARSER_STATE_HEADER_COLON;
                    START_NEW_TOKEN(parser);
                } else {
                    FILL_NEXT_CHAR(parser, ch);
                }
                break;

            case PARSER_STATE_HEADER_COLON:
                EXPECT_CHAR(parser, ch, ' ', PARSER_STATE_HEADER_VALUE);
                break;

            case PARSER_STATE_HEADER_VALUE:
                if (ch == '\r') {
                    FINISH_CUR_TOKEN(parser);
                    parser->req->raw_headers_in[parser->req->raw_header_count].value
                        = parser->_cur_tok;
                    parser->req->raw_header_count++;
                    parser->_state = PARSER_STATE_HEADER_CR;
                    START_NEW_TOKEN(parser);
                } else {
                    FILL_NEXT_CHAR(parser, ch);
                }
                break;

            case PARSER_STATE_HEADER_CR:
                EXPECT_CHAR(parser, ch, '\n', PARSER_STATE_HEADER_LF);
                break;

            case PARSER_STATE_HEADER_LF:
                // Another CR after a header LF, meanning the header end is met.
                if (ch == '\r') { 
                    parser->_state = PARSER_STATE_HEADER_COMPLETE_CR; 
                } else { 
                    parser->_state = PARSER_STATE_HEADER_NAME; 
                    FILL_NEXT_CHAR(parser, ch);
                }
                break;

            case PARSER_STATE_HEADER_COMPLETE_CR:
                EXPECT_CHAR(parser, ch, '\n', PARSER_STATE_COMPLETE);
                break;

            default:
                fprintf(stderr, "Unexpected state: %d\n", parser->_state);
                break;

        }
    }

    *consumed_len = i;

    switch (parser->_state) {
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
