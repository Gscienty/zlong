#ifndef _ZL_SESSION_SESSION_STORAGE_H
#define _ZL_SESSION_SESSION_STORAGE_H

#include "session/http_session.h"

typedef void (*zl_webgateway_enter_fptr) (struct http_session * const session);
typedef size_t
(*zl_websocket_frame_parse_fptr) (struct websocket_frame_parser * const parser,
                                  struct websocket_frame * const frame,
                                  const unsigned char * data,
                                  size_t len);
typedef size_t
(*zl_http_req_protocol_parse_fptr) (struct http_req_parser * const parser,
                                    struct http_req_protocol * const req,
                                    const char * data,
                                    size_t len);

void zl_sessions_init();

void zl_sessions_add(struct http_session * const session);

struct http_session * zl_sessions_find(uv_tcp_t * const tcp_sock);

void zl_sessions_remove(struct http_session * const session);

void zl_sessions_rbnode_init(struct rbnode * const node);


#endif
