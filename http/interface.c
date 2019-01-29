#include "http/web_gateway.h"
#include "http/interface.h"
#include "debug/console.h"
#include <malloc.h>

static struct rbroot __sessions;
static bool __sessions_inited = false;

static void __sessions_init()
{
    if (__sessions_inited)
        return;
    info("sessions init");
    rbtree_root_init(&__sessions);
    __sessions_inited = true;
}

static void __sessions_add(struct http_session_node * const session)
{
    struct rbnode *parent;
    struct rbnode **place;
    struct http_session_node *flag_session;
    uv_tcp_t *flag;

    if (__sessions.root == &__sessions.nil)
        __sessions.root = &session->node;
    else {
        place = &__sessions.root;
        while (*place != &__sessions.nil) {
            parent = *place;
            flag_session = container_of(parent, struct http_session_node, node);
            flag = &flag_session->tcp_sock;

            if (&session->tcp_sock == flag) {
                info("yami");
                return;
            }
            else if (&session->tcp_sock < flag)
                place = &parent->left;
            else 
                place = &parent->right;
        }

        rbtree_link(parent, place, &session->node);
    }

    rbtree_fixup(&__sessions, &session->node);
}

static struct rbnode * __sessions_find(uv_tcp_t * const tcp_sock)
{
    struct rbnode * session_node;
    struct http_session_node * session;
    uv_tcp_t * flag;

    session_node = __sessions.root;
    while (session_node != &__sessions.nil) {
        session = container_of(session_node, struct http_session_node, node);
        flag = &session->tcp_sock;

        if (tcp_sock == flag)
            break;
        else if (tcp_sock < flag)
            session_node = session_node->left;
        else
            session_node = session_node->right;
    }

    return session_node;
}

static void __session_init(struct http_session_node * const session,
                           uv_loop_t * loop)
{
    rbtree_node_init(&__sessions, &session->node);
    zl_http_req_parser_init(&session->parser);
    zl_http_req_protocol_init(&session->req_protocol);
    zl_http_res_protocol_init(&session->res_protocol);
    uv_tcp_init(loop, &session->tcp_sock);

    session->buf = NULL;
    session->buf_size = 0;
}

static void __session_readbuf_alloc(uv_handle_t * handle,
                                    size_t suggested_size,
                                    uv_buf_t * buf)
{
    struct rbnode * session_node = __sessions_find((uv_tcp_t *) handle);
    struct http_session_node * session;
    if (session_node == &__sessions.nil) {
        buf->base = NULL;
        buf->len = 0;
        return;
    }

    session = container_of(session_node, struct http_session_node, node);

    if (session->buf_size < suggested_size) {
        if (session->buf == NULL)
            session->buf = malloc(suggested_size);
        else
            session->buf = realloc(session->buf, suggested_size);
        session->buf_size = suggested_size;
    }
    buf->base = session->buf;
    buf->len = session->buf_size;
}

static void __session_destory(struct http_session_node * const session)
{
    if (session->buf != NULL)
        free(session->buf);

    zl_http_req_protocol_reset(&session->req_protocol);

    rbtree_delete(&__sessions, &session->node);
    free(session);
}


static void __session_close(uv_handle_t * handle)
{
    struct rbnode * session_node;
    struct http_session_node * session;

    session_node = __sessions_find((uv_tcp_t *) handle);
    if (session_node == &__sessions.nil)
        return;

    session = container_of(session_node, struct http_session_node, node);
    __session_destory(session);
}

static void __session_write(uv_write_t *req, int status)
{
    (void) req;
    if (status < 0)
        return;
}

static bool __session_respond(uv_stream_t * stream,
                              struct http_session_node * const session)
{
    uv_buf_t buf;
    
    if (!zl_http_res_protocol_serialize(&session->res_protocol)) {
        return false;
    }

    buf.base = session->res_protocol.tcp_payload;
    buf.len  = session->res_protocol.tcp_payload_writable;

    session->res_protocol.tcp_payload          = NULL;
    session->res_protocol.tcp_payload_size     = 0;
    session->res_protocol.tcp_payload_writable = 0;

    uv_write(&session->writer, stream, &buf, 1, __session_write);
    return true;
}

static void __session_read(uv_stream_t * stream,
                           ssize_t nread,
                           const uv_buf_t * buf)
{
    struct rbnode * session_node;
    struct http_session_node *session;

    session_node = __sessions_find((uv_tcp_t *) stream);
    if (session_node == &__sessions.nil) {
        error("session not found");
        return;
    }
    session = container_of(session_node, struct http_session_node, node);


    if (nread <= 0 && nread != EAGAIN) {
        info("close session[%x]", session);
        uv_close((uv_handle_t *) stream, __session_close);
        return;
    }

    info("parse http request: %x", session);

    zl_http_req_protocol_parse(&session->parser,
                               &session->req_protocol,
                               buf->base,
                               nread);

    if (session->parser.stat == HTTP_REQ_STAT_END) {
        zl_webgateway_enter(&session->req_protocol, &session->res_protocol);

        zl_http_req_parser_init(&session->parser);
        zl_http_req_protocol_reset(&session->req_protocol);

        if (!__session_respond(stream, session))
            uv_close((uv_handle_t *) stream, __session_close);

        zl_http_res_protocol_clear(&session->res_protocol);
    }
}

static void __new_session(uv_stream_t *server, int status)
{
    int ret;
    if (status < 0) {
        error("new session error: %s", uv_strerror(status));
        return;
    }
    __sessions_init();

    struct http_session_node *session =
        (struct http_session_node *) malloc(sizeof(struct http_session_node));
    if (session == NULL) {
        error("new session error: malloc failed");
        return;
    }
    __session_init(session, server->loop);

    ret = uv_accept(server, (uv_stream_t *) &session->tcp_sock);
    if (ret < 0) {
        error("new session error: accept failed");
        return;
    }

    info("accept a newly session[%x]", session);
    __sessions_add(session);

    uv_read_start((uv_stream_t *) &session->tcp_sock,
                  __session_readbuf_alloc,
                  __session_read);
}

/**
 * init http
 * @param http
 * 
 */
int zl_http_init(struct http * const http)
{
    int ret;
    ret = uv_loop_init(&http->event_looper);
    if (ret < 0) {
        error("loop init error");
        return -1;
    }

    ret = uv_tcp_init(&http->event_looper, &http->tcp_handler);
    if (ret < 0) {
        error("tcp server init error");
        return ret;
    }
    if (http->delay != 0) {
        ret = uv_tcp_keepalive(&http->tcp_handler, 1, http->delay);
        if (ret < 0) {
            error("keepalive error");
            return ret;
        }
    }
    ret = uv_tcp_bind(&http->tcp_handler, (struct sockaddr *) &http->addr, 0);
    if (ret < 0) {
        error("tcp bind failed");
        return ret;
    }
    ret = uv_listen((uv_stream_t *) &http->tcp_handler,
                    http->backlog,
                    __new_session);
    if (ret < 0) {
        error("listen failed");
        return ret;
    }

    info("http init success.");

    return ret;
}

/**
 * set no delay
 * @param http
 * 
 */
int zl_http_nodelay(struct http * const http)
{
    return uv_tcp_nodelay(&http->tcp_handler, 1);
}

/**
 * simultaneous accepts
 * @param http
 * 
 */
int zl_http_simultaneous(struct http * const http)
{
    return uv_tcp_simultaneous_accepts(&http->tcp_handler, 1);
}

/**
 * run
 * @param http
 * 
 */
int zl_http_run(struct http * const http)
{
    return uv_run(&http->event_looper, UV_RUN_DEFAULT);
}
