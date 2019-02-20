#include "http/config.h"
#include <stddef.h>

static struct config __config;

/**
 * init global config
 * 
 */
void zl_config_init()
{
    __config.addr        = NULL;
    __config.port        = 0;
    __config.script_path = NULL;
    __config.route_path  = NULL;
    __config.security    = false;
    __config.cert        = NULL;
    __config.key         = NULL;
}

/**
 * get config
 * 
 */
struct config * zl_config_get()
{
    return &__config;
}
