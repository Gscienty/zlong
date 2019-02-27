#include "debug/console.h"
#include "lua_engine/router.h"
#include "http/config.h"
#include "utils/kv_param.h"
#include <string.h>
#include <malloc.h>

static const char * __get_path(const char * uri)
{
    char * delimiter;
    char * path;
    char * chr;
    int i;

    path = calloc(256, 1);
    if (path == NULL) {
        error("malloc failed");
        return NULL;
    }

    delimiter = strchr(uri, '?');
    if (delimiter == NULL) {
        strcpy(path, uri + 1);
    }
    else {
        chr = (char *) uri;
        for (i = 1; chr != delimiter; i++, chr++) {
            path[i] = *chr;
        }
    }

    return path;
}

const char * zl_lua_engine_get_script_path(struct http_req_protocol * const req)
{
    return __get_path(req->uri);
}

void zl_lua_engine_notfound(struct http_res_protocol * const res)
{
    struct kv_param * kv;
    res->status_code = 404;

    kv = malloc(sizeof(struct kv_param));
    if (kv == NULL) {
        error("malloc failed");
        return;
    }

    zl_http_res_protocol_add_param(res,
                                   strdup("Content-Type"),
                                   strdup("text/plain"));

    res->payload = strdup("zlong not found");
    res->payload_size = strlen(res->payload);
}
