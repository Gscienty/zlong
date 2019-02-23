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
                                    const unsigned char * data,
                                    size_t len);

typedef void (*zl_session_foreachor_fptr) (struct http_session * const session,
                                           void * arg);

void zl_sessions_init();

void zl_sessions_add(struct http_session * const session);

void zl_sessions_remove(struct http_session * const session);

void zl_sessions_rbnode_init(struct rbnode * const node);

void zl_sessions_foreach(zl_session_foreachor_fptr fptr, void * arg);


#endif
