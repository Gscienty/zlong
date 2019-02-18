#include "debug/console.h"
#include "session/session_storage.h"
#include <malloc.h>

static struct rbroot __sessions;
static bool __sessions_inited = false;


void zl_sessions_init()
{
    if (__sessions_inited)
        return;
    info("sessions init");
    rbtree_root_init(&__sessions);
    __sessions_inited = true;
}

void zl_sessions_add(struct http_session * const session)
{
    struct rbnode *parent;
    struct rbnode **place;
    struct http_session *flag_session;
    uv_tcp_t *flag;

    if (__sessions.root == &__sessions.nil)
        __sessions.root = &session->node;
    else {
        place = &__sessions.root;
        while (*place != &__sessions.nil) {
            parent = *place;
            flag_session = container_of(parent, struct http_session, node);
            flag = &flag_session->tcp_sock;

            if (&session->tcp_sock == flag) {
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

struct http_session * zl_sessions_find(uv_tcp_t * const tcp_sock)
{
    struct rbnode * session_node;
    struct http_session * session;
    uv_tcp_t * flag;

    session_node = __sessions.root;
    while (session_node != &__sessions.nil) {
        session = container_of(session_node, struct http_session, node);
        flag = &session->tcp_sock;

        if (tcp_sock == flag)
            break;
        else if (tcp_sock < flag)
            session_node = session_node->left;
        else
            session_node = session_node->right;
    }

    if (session_node == &__sessions.nil) {
        return NULL;
    }
    return container_of(session_node, struct http_session, node);
}

void zl_sessions_remove(struct http_session * const session)
{
    rbtree_delete(&__sessions, &session->node);
}

void zl_sessions_rbnode_init(struct rbnode * const node)
{
    rbtree_node_init(&__sessions, node);
}
