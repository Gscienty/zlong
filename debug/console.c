#include "debug/console.h"
#include <stddef.h>
#include <malloc.h>

static struct zl_debug_config *__config = NULL;

/**
 * init debug config
 * 
 */
int zl_debug_config_init()
{
    __config = (struct zl_debug_config *)
        malloc(sizeof(struct zl_debug_config));
    if (__config == NULL)
        return -1;

    __config->error_switch = false;
    __config->info_switch  = false;
    __config->warn_switch  = false;

    return 0;
}

static int __config_switch_field(bool *field, bool value)
{
    *field = value;
    return 0;
}

/**
 * open info switch
 * 
 */
int zl_debug_config_info_switch(bool open_flag)
{
    if (__config == NULL)
        return -1;

    return __config_switch_field(&__config->info_switch, open_flag);
}

/**
 * switch warn 
 * @param open_flag
 * 
 */
int zl_debug_config_warn_switch(bool open_flag)
{
    if (__config == NULL)
        return -1;

    return __config_switch_field(&__config->warn_switch, open_flag);
}

/**
 * switch error
 * @param open_flag
 * 
 */
int zl_debug_config_error_switch(bool open_flag)
{
    if (__config == NULL)
        return -1;

    return __config_switch_field(&__config->error_switch, open_flag);
}
