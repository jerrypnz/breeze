#include "mod.h"
#include "mod_static.h"
#include "common.h"
#include "log.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

#define MAX_EXPIRE_HOURS = 87600

#define FILE_TYPE_COUNT 10

typedef struct _mime_type {
    char *content_type;
    char *exts[FILE_TYPE_COUNT];
} mime_type_t;

mime_type_t standard_types[] = {
    {"text/html",                {"html", "htm", "shtml", NULL}},
    {"text/css",                 {"css", NULL}},
    {"text/xml",                 {"xml", NULL}},
    {"text/plain",               {"txt", NULL}},
    {"image/png",                {"png", NULL}},
    {"image/gif",                {"gif", NULL}},
    {"image/tiff",               {"tif", "tiff", NULL}},
    {"image/jpeg",               {"jpg", "jpeg", NULL}},
    {"image/x-ms-bmp",           {"bmp", NULL}},
    {"image/svg+xml",            {"svg", "svgz", NULL}},
    {"application/x-javascript", {"js", NULL}}
};

static struct hsearch_data std_mime_type_hash;

/*
 * dir_filter uses this flag to decide whether to filter out
 * hidden files (dot files). Since C does not have closure, we
 * have to use a global variable to hack it.
 * It is set to the value of a
 * mod_static_conf_t.show_hidden_file before listing the file.
 */
static int show_hidden_file = 0;

static void *mod_static_conf_create(json_value *conf_value);
static void  mod_static_conf_destroy(void *conf);

static int static_file_write_content(request_t *req,
                                     response_t *resp,
                                     handler_ctx_t *ctx);
static int static_file_cleanup(request_t *req,
                               response_t *resp,
                               handler_ctx_t *ctx);

static int static_file_handle_error(response_t *resp, int fd);
static void handle_content_type(response_t *resp, const char *filepath);
static int handle_cache(request_t *req, response_t *resp,
                        const struct stat *st, const mod_static_conf_t *conf);
static int handle_range(request_t *req, response_t *resp,
                        size_t *offset, size_t *size);
static char* generate_etag(const struct stat *st);
static int try_open_file(const char *path, int *fd, struct stat *st);
static int static_file_listdir(response_t *resp, const char *path,
                               const char *realpath);
static int dir_filter(const struct dirent *ent);

/* Module descriptor */
module_t mod_static = {
    "static",
    mod_static_init,
    mod_static_conf_create,
    mod_static_conf_destroy,
    static_file_handle
};

int mod_static_init() {
    int i;
    int j;
    size_t size;
    ENTRY item, *ret;
    char **ext = NULL;

    size = sizeof(standard_types) / sizeof(standard_types[0]);

    bzero(&std_mime_type_hash, sizeof(struct hsearch_data));
    if (hcreate_r(size * 2, &std_mime_type_hash) == 0) {
        error("Error creating standard MIME type hash");
        return -1;
    }
    for (i = 0; i < size; i++) {
        for (ext = standard_types[i].exts, j = 0;
             *ext != NULL && j < FILE_TYPE_COUNT;
             ext++, j++) {
            item.key = *ext;
            item.data = standard_types[i].content_type;
            debug("Registering standard MIME type %s:%s",
                  *ext, standard_types[i].content_type);
            if (hsearch_r(item, ENTER, &ret, &std_mime_type_hash) == 0) {
                error("Error entering standard MIME type");
            }
        }
    }
    return 0;
}

static void *mod_static_conf_create(json_value *conf_value) {
    mod_static_conf_t *conf = NULL;
    DECLARE_CONF_VARIABLES()

    conf = (mod_static_conf_t*) calloc(1, sizeof(mod_static_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    BEGIN_CONF_HANDLE(conf_value)
    ON_STRING_CONF("root", conf->root)
    ON_STRING_CONF("index", conf->index)
    ON_BOOLEAN_CONF("list_dir", conf->enable_list_dir)
    ON_INTEGER_CONF("expires", conf->expire_hours)
    ON_BOOLEAN_CONF("etag", conf->enable_etag)
    ON_BOOLEAN_CONF("range_request", conf->enable_range_req)
    END_CONF_HANDLE()
    return conf;
}

static void  mod_static_conf_destroy(void *conf) {
    free(conf);
}

static int try_open_file(const char *path, int *fdptr, struct stat *st) {
    int fd, res;

    debug("Try opening file: %s", path);
    res = stat(path, st);
    if (res < 0) {
        return -1;
    }

    if (S_ISDIR(st->st_mode)) {
        return 1;
    } else if (S_ISREG(st->st_mode)) {
        fd = open(path, O_RDONLY);
        if (fd < 0) {
            return -1;
        }
        *fdptr = fd;
        return 0;
    } else {
        return -1;
    }
}

const char *listdir_header =
    "<html>"
    "<head><title>Index of %s</title></head>"
    "<body>"
    "<h1>Index of %s</h1>"
    "<hr/>"
    "<pre>";

const char *listdir_file = "<a href=\"%s\">%s</a>\r\n";
const char *listdir_dir = "<a href=\"%s/\">%s/</a>\r\n";

const char *listdir_footer =
    "</pre>"
    "<hr/>"
    "<p>Powered by "
    "<a href=\"https://github.com/moonranger/breeze\" target=\"_blank\">%s</a>"
    "</p>"
    "</body>"
    "</html>";

static int static_file_listdir(response_t *resp, const char *path,
                               const char *realpath) {
    struct dirent **ent_list, *ent;
    int    ent_len;
    char   buf[2048];
    int    pos = 0, i;

    debug("Opening dir: %s", realpath);
    if ((ent_len = scandir(realpath, &ent_list, dir_filter, versionsort)) < 0) {
        return static_file_handle_error(resp, -1);
    }
    resp->status = STATUS_OK;
    resp->connection = CONN_CLOSE;
    response_set_header(resp, "Content-Type", "text/html; charset=UTF-8");
    response_send_headers(resp, NULL);
    pos += snprintf(buf, 2048, listdir_header, path, path);
    for (i = 0; i < ent_len; i++) {
        ent = ent_list[i];
        pos += snprintf(buf + pos, 2048 - pos,
                        ent->d_type == DT_DIR ? listdir_dir : listdir_file,
                        ent->d_name, ent->d_name);
        if (2048 - pos < 255) {
            response_write(resp, buf, pos, NULL);
            pos = 0;
        }
        free(ent);
    }
    free(ent_list);
    pos += snprintf(buf + pos, 2048 - pos, listdir_footer, _BREEZE_NAME);
    response_write(resp, buf, pos, NULL);

    return HANDLER_DONE;
}

static int dir_filter(const struct dirent *ent) {
    const char *name = ent->d_name;
    if (name[0] == '.') {
        if (name[1] == '\0')
            return 0; // Skip "."
        else if (name[1] == '.' && name[2] == '\0')
            return 1; // Show ".." for parent dir
        else if (!show_hidden_file)
            return 0;
    }
    return 1;
}

int static_file_handle(request_t *req, response_t *resp,
                       handler_ctx_t *ctx) {
    mod_static_conf_t *conf;
    char              path[2048];
    int               fd = -1, res, use_301;
    struct stat       st;
    size_t            len, pathlen, filesize, fileoffset;
    ctx_state_t       val;

    conf = (mod_static_conf_t*) ctx->conf;
    len = strlen(conf->root);
    strncpy(path, conf->root, 2048);
    if (path[len - 1] == '/') {
        path[len - 1] = '\0';
    }
    if (req->path[0] != '/') {
        return response_send_status(resp, STATUS_BAD_REQUEST);
    }
    strncat(path, req->path, 2048 - len);
    debug("Request path: %s, real file path: %s", req->path, path);
    res = try_open_file(path, &fd, &st);
    if (res < 0) {
        return static_file_handle_error(resp, fd);
    } else if (res > 0) { // Is a directory, try index files.
        pathlen = strlen(path);
        use_301 = 0;
        if (path[pathlen - 1] != '/') {
            path[pathlen] = '/';
            path[pathlen + 1] = '\0';
            pathlen++;
            use_301 = 1;
        }
        //for (i = 0; i < 10 && res != 0 && conf->index[i] != NULL; i++) {
        //    path[pathlen] = '\0';
        //    strncat(path, conf->index[i], 2048 - pathlen);
        //    res = try_open_file(path, &fd, &st);
        //}
        path[pathlen] = '\0';
        strncat(path, conf->index, 2048 - pathlen);
        res = try_open_file(path, &fd, &st);
        path[pathlen] = '\0';
        if (res != 0) {
            if (conf->enable_list_dir) {
                if (use_301) {
                    // TODO Support HTTPS
                    snprintf(path, 2048, "http://%s%s/", req->host, req->path);
                    response_set_header(resp, "Location", path);
                    resp->status = STATUS_MOVED;
                    resp->content_length = 0;
                    response_send_headers(resp, NULL);
                    return HANDLER_DONE;
                }
                show_hidden_file = conf->show_hidden_file;
                return static_file_listdir(resp, req->path, path);
            } else {
                return static_file_handle_error(resp, fd);
            }
        }
    }

    fileoffset = 0;
    filesize = st.st_size;
    res = handle_range(req, resp, &fileoffset, &filesize);
    if (res < 0) {
        resp->status = STATUS_OK;
    } else if (res == 0) {
        resp->status = STATUS_PARTIAL_CONTENT;
    } else {
        return response_send_status(resp, STATUS_RANGE_NOT_SATISFIABLE);
    }

    resp->content_length = filesize;
    handle_content_type(resp, path);
    if (handle_cache(req, resp, &st, conf)) {
        return response_send_status(resp, STATUS_NOT_MODIFIED);
    }
    
    val.as_int = fd;
    context_push(ctx, val);
    val.as_long = fileoffset;
    context_push(ctx, val);
    val.as_long = filesize;
    context_push(ctx, val);
    debug("sending headers");
    response_send_headers(resp, static_file_write_content);
    return HANDLER_UNFISHED;
}

/*
 * Return values:
 *  0:  valid range request
 *  1:  range not satisfiable (should return 416)
 *  -1: syntactically invalid range (ignore, return full content)
 */
static int handle_range(request_t *req, response_t *resp,
                        size_t *offset, size_t *size) {
    const char   *range_spec;
    char         buf[100], *pos;
    size_t       len, total_size = *size, off, sz, end;
    int          idx;

    range_spec = request_get_header(req, "range");
    if (range_spec == NULL) {
        return -1;
    }
    len = strlen(range_spec);
    if (len < 8) {
        return -1;
    }
    if (strstr(range_spec, "bytes=") != range_spec) {
        error("Only byte ranges are supported(error range:%s)", range_spec);
        return -1;
    }
    strncpy(buf, range_spec + 6, 100);
    len = strlen(buf);
    if (index(buf, ',') != NULL) {
        error("Multiple ranges are not supported.");
        return -1;
    }
    pos = index(buf, '-');
    if (pos == NULL) {
        error("Invalid range spec: %s.", range_spec);
        return -1;
    }
    idx = pos - buf;
    if (idx == 0) {
        sz = atol(buf + 1);
        end = total_size;
        off = total_size - sz;
    } else if (idx == len - 1) {
        buf[idx] = '\0';
        off = atol(buf);
        end = total_size;
        sz = total_size - off;
    } else {
        buf[idx] = '\0';
        off = atol(buf);
        end = atol(buf + idx + 1);
        sz = end - off + 1;
    }

    if (end < off)
        return -1;
    if (off >= total_size || sz > total_size)
        return 1;

    response_set_header_printf(resp, "Content-Range", "bytes %ld-%ld/%ld",
                               off, off + sz - 1, total_size);
    *offset = off;
    *size = sz;
    return 0;
}

static void handle_content_type(response_t *resp, const char *filepath) {
    char  *content_type = NULL, ext[20];
    int   dot_pos = -1;
    int   i;
    size_t  len = strlen(filepath);
    ENTRY   item, *ret;

    for (i = len - 1; i > 0; i--) {
        if (filepath[i] == '.') {
            dot_pos = i;
            break;
        }
    }

    if (dot_pos < 0) {
        // No '.' found in the file name (no extension part)
        return;
    }

    strncpy(ext, filepath + 1 + dot_pos, 20);
    strlowercase(ext, ext, 20);
    debug("File extension: %s", ext);
    
    item.key = ext;
    if (hsearch_r(item, FIND, &ret, &std_mime_type_hash) == 0) {
        return;
    }
    content_type = (char*) ret->data;
    if (content_type != NULL) {
        debug("Content type: %s", content_type);
        response_set_header(resp, "Content-Type", content_type); 
    }
}

static int handle_cache(request_t *req, response_t *resp,
                        const struct stat *st, const mod_static_conf_t *conf) {
    const char   *if_mod_since, *if_none_match;
    char   *etag;
    time_t  mtime, req_mtime;
    int    not_modified = 0;
    char   *buf;
    char   *cache_control;

    mtime = st->st_mtime;
    if_mod_since = request_get_header(req, "if-modified-since");
    if (if_mod_since != NULL &&
        parse_http_date(if_mod_since, &req_mtime) == 0 &&
        req_mtime == mtime) {
        debug("Resource not modified");
        not_modified = 1;
    }
    buf = response_alloc(resp, 32);
    format_http_date(&mtime, buf, 32);
    response_set_header(resp, "Last-Modified", buf);

    if (conf->enable_etag) {
        etag = generate_etag(st);
        if (not_modified) {
            if_none_match = request_get_header(req, "if-none-match");
            if (if_none_match == NULL ||
                strcmp(etag, if_none_match) != 0) {
                not_modified = 0;
            }
        }
        response_set_header(resp, "ETag", etag);
    }

    if (conf->expire_hours >= 0) {
        buf = response_alloc(resp, 32);
        mtime += conf->expire_hours * 3600;
        format_http_date(&mtime, buf, 32);
        response_set_header(resp, "Expires", buf);
        cache_control = response_alloc(resp, 20);
        snprintf(cache_control, 20, "max-age=%d", conf->expire_hours * 3600);
    } else {
        cache_control = "no-cache";
    }
    response_set_header(resp, "Cache-Control", cache_control);
    return not_modified;
}

static char* generate_etag(const struct stat *st) {
    char tag_buf[128];
    snprintf(tag_buf, 128, "etag-%ld-%zu", st->st_mtime, st->st_size);
    return crypt(tag_buf, "$1$breeze") + 10; // Skip the $id$salt part
}

static int static_file_write_content(request_t *req, response_t *resp, handler_ctx_t *ctx) {
    int fd;
    size_t size, offset;

    size = context_pop(ctx)->as_long;
    offset = context_pop(ctx)->as_long;
    fd = context_peek(ctx)->as_int;
    debug("writing file");
    if (response_send_file(resp, fd, offset, size, static_file_cleanup) < 0) {
        error("Error sending file");
        return response_send_status(resp, STATUS_NOT_FOUND);
    }
    return HANDLER_UNFISHED;
}

static int static_file_cleanup(request_t *req, response_t *resp, handler_ctx_t *ctx) {
    int fd;

    debug("cleaning up");
    fd = context_pop(ctx)->as_int;
    close(fd);
    return HANDLER_DONE;
}

static int static_file_handle_error(response_t *resp, int fd) {
    int err = errno;
    if (fd > 0)
        (void) close(fd); // Don't care the failure
    switch(err) {
    case EACCES:
    case EISDIR:
        return response_send_status(resp, STATUS_FORBIDDEN);
    case ENOENT:
    default:
        return response_send_status(resp, STATUS_NOT_FOUND);
    }
}


