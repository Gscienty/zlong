#ifndef _ZL_HTTP_CONFIG_H
#define _ZL_HTTP_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

struct config {
    const char * addr;
    uint16_t port;
    const char * script_path;
    const char * route_path;
    bool security;
    const char * cert;
    const char * key;
    int lifetime;
};

/**
 * init global config
 * 
 */
void zl_config_init();

/**
 * get config
 * 
 */
struct config * zl_config_get();

#endif
