#ifndef _ZL_SESSION_SESSION_HANDLER_H
#define _ZL_SESSION_SESSION_HANDLER_H

#include "session/session_storage.h"
#include <openssl/ssl.h>

void zl_session_destory(struct http_session * const session);

void zl_session_readbuf_alloc(uv_handle_t * handle,
                              size_t suggested_size,
                              uv_buf_t * buf);

void zl_session_close(uv_handle_t * handle);

bool zl_session_http_respond(uv_stream_t * stream,
                             struct http_session * const session);

void zl_session_websocket_reset(struct http_session * session);

void zl_session_read(uv_stream_t * stream,
                     ssize_t nread,
                     const uv_buf_t * buf);

void zl_session_new(uv_stream_t *server, int status);

void zl_session_register_webgetway_enter(zl_webgateway_enter_fptr fptr);

void zl_session_register_websocket_parser(zl_websocket_frame_parse_fptr fptr);

void zl_session_register_req_protocol_parser(zl_http_req_protocol_parse_fptr fptr);


#endif
