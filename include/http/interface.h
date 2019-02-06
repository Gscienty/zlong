#ifndef _ZL_HTTP_INTERFACE_H
#define _ZL_HTTP_INTERFACE_H

#include "session/session_storage.h"
#include <uv.h>

/**
 * init http
 * @param http
 * @param fptr
 * 
 */
int zl_http_init(struct http * const http);

/**
 * set no delay
 * @param http
 * 
 */
int zl_http_nodelay(struct http * const http);

/**
 * set keep alive
 * @param http
 * 
 */
int zl_http_keeplive(struct http * const http);

/**
 * simultaneous accepts
 * @param http
 * 
 */
int zl_http_simultaneous(struct http * const http);

/**
 * run
 * @param http
 * 
 */
int zl_http_run(struct http * const http);

#endif
