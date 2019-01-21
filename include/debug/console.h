#ifndef _DEBUG_CONSOLE_H
#define _DEBUG_CONSOLE_H

#include <stdbool.h>

struct zl_debug_config {
    bool info_switch;
    bool warn_switch;
    bool error_switch;
};

/**
 * init debug config
 * 
 */
int zl_debug_config_init();

/**
 * switch info
 * @param open_flag
 * 
 */
int zl_debug_config_info_switch(bool open_flag);

/**
 * switch warn 
 * @param open_flag
 * 
 */
int zl_debug_config_warn_switch(bool open_flag);

/**
 * switch error
 * @param open_flag
 * 
 */
int zl_debug_config_error_switch(bool open_flag);


#endif
