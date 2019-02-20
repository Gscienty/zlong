#include "cargs.h"
#include "debug/console.h"
#include "http/interface.h"
#include "http/config.h"
#include "server/main.h"

cargs_flag(https_arg_flag, __SET__("https"));

cargs_subarg(https_arg_flag, https_arg_addr,
             __SET__("--addr"),
             __SET__(arg_type_string));
cargs_subarg(https_arg_flag, https_arg_port,
             __SET__("--port"),
             __SET__(arg_type_short));
cargs_subarg(https_arg_flag, https_arg_path,
             __SET__("--scripts"),
             __SET__(arg_type_string));
cargs_subarg(https_arg_flag, https_arg_cert,
             __SET__("--cert"),
             __SET__(arg_type_string));
cargs_subarg(https_arg_flag, https_arg_private_key,
             __SET__("--private-key"),
             __SET__(arg_type_string));

cargs_process(https_process,
              __SET__(cargs_theme_arg(https_arg_addr, true),
                      cargs_theme_arg(https_arg_port, true),
                      cargs_theme_arg(https_arg_path, true),
                      cargs_theme_arg(https_arg_cert, true),
                      cargs_theme_arg(https_arg_private_key, true)),
              const char * addr,
              unsigned short port,
              const char * path,
              const char * cert,
              const char * key)
{
    struct config * config;

    zl_config_init();
    config = zl_config_get();

    config->addr = addr;
    config->port = port;
    config->script_path = path;
    config->cert = cert;
    config->key = key;
    config->security = true;

    http_common_runner();
}
