#include "session/session_handler.h"
#include "http/websocket.h"
#include "http/web_gateway.h"
#include "http/interface.h"
#include "http/config.h"
#include "debug/console.h"
#include <malloc.h>
#include <openssl/engine.h>

static void __ssl_init(struct http * const http)
{
    struct config * config = zl_config_get();

    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    http->ctx = SSL_CTX_new(TLSv1_2_server_method());
    if (http->ctx == NULL) {
        error("SSL_CTX_new failed");
    }
    SSL_CTX_set_options(http->ctx,
                        SSL_OP_ALL
                        | SSL_OP_NO_SSLv2
                        | SSL_OP_NO_SSLv3
                        | SSL_OP_NO_COMPRESSION
                        | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
    SSL_CTX_set_ecdh_auto(http->ctx, 1);
    if (SSL_CTX_use_certificate_file(http->ctx, config->cert, SSL_FILETYPE_PEM) != 1) {
        error("SSL_CTX_use_certificate_file failed");
    }
    if (SSL_CTX_use_PrivateKey_file(http->ctx, config->key, SSL_FILETYPE_PEM) != 1) {
        error("SSL_CTX_use_PrivateKey_file failed");
    }
    if (SSL_CTX_check_private_key(http->ctx) != 1) {
        error("SSL_CTX_check_private_key failed");
    }
}

/**
 * init http
 * @param http
 * 
 */
int zl_http_init(struct http * const http)
{
    struct config * config = zl_config_get();
    int ret;
    ret = uv_loop_init(&http->event_looper);
    if (ret < 0) {
        error("loop init error");
        return -1;
    }

    zl_session_register_webgetway_enter(zl_webgateway_enter);
    zl_session_register_websocket_parser(zl_websocket_frame_parse);
    zl_session_register_req_protocol_parser(zl_http_req_protocol_parse);

    if (config->security) {
        info("init ssl ctx");
        __ssl_init(http);
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
                    zl_session_new);
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
