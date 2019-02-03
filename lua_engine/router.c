#include "debug/console.h"
#include "lua_engine/router.h"
#include "http/config.h"
#include "utils/kv_param.h"
#include <string.h>
#include <malloc.h>

static const char * __get_path(const struct config * const config,
                               const char * uri)
{
    char * delimiter;
    char * path;
    char * chr;
    int i;
    int base_len;

    path = calloc(256, 1);
    if (path == NULL) {
        error("malloc failed");
        return NULL;
    }

    strcpy(path, config->script_path);
    base_len = strlen(config->script_path);

    delimiter = strchr(uri, '?');
    if (delimiter == NULL) {
        strcpy(path + base_len, uri);
    }
    else {
        chr = (char *) uri;
        for (i = base_len; chr != delimiter; i++, chr++) {
            path[i] = *chr;
        }
    }

    return path;
}

const char * zl_lua_engine_get_script_path(struct http_req_protocol * const req)
{
    const struct config * config = zl_config_get();

    if (config->route_path == NULL) {
        return __get_path(config, req->uri);
    }

    return NULL;
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
