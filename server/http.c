#include "cargs.h"
#include "debug/console.h"
#include "http/interface.h"

cargs_flag(http_arg_flag, __SET__("http"));

cargs_subarg(http_arg_flag, http_arg_host,
             __SET__("--host"),
             __SET__(arg_type_string));
cargs_subarg(http_arg_flag, http_arg_port,
             __SET__("--port"),
             __SET__(arg_type_short));

cargs_process(http_process,
              __SET__(cargs_theme_arg(http_arg_host, true),
                      cargs_theme_arg(http_arg_port, true)),
              const char * host, unsigned short port)
{
    struct http http;
    http.addr.v4.sin_family = AF_INET;
    http.addr.v4.sin_addr.s_addr = inet_addr(host);
    http.addr.v4.sin_port = htons(port);

    http.backlog = 10;
    http.delay = 0;

    int ret;
    ret = zl_http_init(&http);
    if (ret < 0)
        error("init");
    info("init success");
    ret = zl_http_run(&http);
    if (ret < 0)
        error("run");
    info("run success");
}
