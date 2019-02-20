#ifndef _ZL_DEBUG_CONSOLE_H
#define _ZL_DEBUG_CONSOLE_H

#include <stdbool.h>
#include <stdio.h>

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

/**
 * get info status
 * 
 */
bool zl_debug_config_info();

/**
 * get warn status
 *
 */
bool zl_debug_config_warn();

/**
 * get error status
 *
 */
bool zl_debug_config_error();

#define ZL_CONL_COLOR_RED    "\033[1;31m"
#define ZL_CONL_COLOR_WHITE  "\033[1;37m"
#define ZL_CONL_COLOR_YELLOW "\033[1;33m"

#ifdef DEBUG

#define info(format, ...) ({ \
    if (zl_debug_config_info()) \
        fprintf(stderr, \
                "[INFO]" \
                " file: \"" __FILE__ "\". " format "\n", \
                ##__VA_ARGS__); \
                           })


#define warn(format, ...) ({ \
    if (zl_debug_config_warn()) \
        fprintf(stderr, \
                ZL_CONL_COLOR_YELLOW "[WARN]" ZL_CONL_COLOR_WHITE \
                " file: \"" __FILE__ "\". " format "\n", \
                ##__VA_ARGS__); \
                           })


#define error(format, ...) ({ \
    if (zl_debug_config_error()) \
        fprintf(stderr, \
                ZL_CONL_COLOR_RED "[ERROR]" ZL_CONL_COLOR_WHITE \
                " file: \"" __FILE__ "\". " format "\n", \
                ##__VA_ARGS__); \
                            })

#else

#define info(format, ...)
#define warn(format, ...)
#define error(format, ...)

#endif

#endif
