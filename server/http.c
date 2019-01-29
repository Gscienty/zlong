#include "cargs.h"
#include "debug/console.h"
#include "http/interface.h"
#include "http/config.h"
#include "server/main.h"

cargs_flag(http_arg_flag, __SET__("http"));

cargs_subarg(http_arg_flag, http_arg_addr,
             __SET__("--addr"),
             __SET__(arg_type_string));
cargs_subarg(http_arg_flag, http_arg_port,
             __SET__("--port"),
             __SET__(arg_type_short));
cargs_subarg(http_arg_flag, http_arg_path,
             __SET__("--scripts"),
             __SET__(arg_type_string));

cargs_process(http_process,
              __SET__(cargs_theme_arg(http_arg_addr, true),
                      cargs_theme_arg(http_arg_port, true),
                      cargs_theme_arg(http_arg_path, true)),
              const char * addr, unsigned short port,
              const char * path)
{
    struct config * config;

    zl_config_init();
    config = zl_config_get();

    config->addr = addr;
    config->port = port;
    config->script_path = path;

    http_common_runner();
}
