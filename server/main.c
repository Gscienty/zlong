#include "http/interface.h"
#include "cargs.h" 
#include "server/main.h"
#include "debug/console.h"
#include <unistd.h>
#ifdef DEBUG
#include <mcheck.h>
#endif

/**
 * run common http
 * 
 */
void http_common_runner()
{
    int ret;
    struct http http;
    struct config * config = zl_config_get();

    http.addr.v4.sin_family = AF_INET;
    http.addr.v4.sin_addr.s_addr = inet_addr(config->addr);
    http.addr.v4.sin_port = htons(config->port);

    http.backlog = 10;
    http.delay = config->lifetime;

    chdir(config->script_path);

    ret = zl_http_init(&http);
    if (ret < 0)
        return;

    zl_http_run(&http);
}

int main(int argc, char ** argv)
{
#ifdef DEBUG
    setenv("MALLOC_TRACE", "trace.log", 1);
    mtrace();
#endif
    zl_debug_config_init();
    zl_debug_config_info_switch(true);
    zl_debug_config_error_switch(true);

    return cargs_run(argc, argv, NULL);
}
