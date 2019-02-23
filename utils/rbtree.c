#include "utils/rbtree.h"
#include <stddef.h>
#include <malloc.h>

/**
 * init rbtree root
 * @param: root
 * 
 */
void rbtree_root_init(struct rbroot * const root)
{
    root->nil.color  = RBTREE_BLACK;
    root->nil.left   = &root->nil;
    root->nil.right  = &root->nil;
    root->nil.parent = &root->nil;
    root->root       = &root->nil;
}

/**
 * init rbtree node
 * @param root: rbtree root node
 * @param node: rbtree node
 * 
 */
void rbtree_node_init(const struct rbroot * const root,
                      struct rbnode * const node)
{
    node->color  = RBTREE_RED;
    node->left   = (struct rbnode *) &root->nil;
    node->right  = (struct rbnode *) &root->nil;
    node->parent = (struct rbnode *) &root->nil;
}

static void __rbtree_left_rotate(struct rbroot * const root, struct rbnode *node)
{
    struct rbnode *right = node->right;
    node->right = right->left;
    if (right->left != &root->nil)
        right->left->parent = node;
    right->parent = node->parent;
    if (node->parent == &root->nil)
        root->root = right;
    else if (node == node->parent->left)
        node->parent->left = right;
    else
        node->parent->right = right;
    right->left = node;
    node->parent = right;
}

static void __rbtree_right_rotate(struct rbroot * const root, struct rbnode *node)
{
    struct rbnode *left = node->left;
    node->left = left->right;
    if (left->right != &root->nil)
        left->right->parent = node;
    left->parent = node->parent;
    if (node->parent == &root->nil)
        root->root = left;
    else if (node == node->parent->left)
        node->parent->left = left;
    else
        node->parent->right = left;
    left->right = node;
    node->parent = left;
}

/**
 * fix newly rbtree node
 * @param root: rbtree root
 * @param node: newly rbtree node
 */
void rbtree_fixup(struct rbroot * const root, struct rbnode *node)
{
    struct rbnode *uncle;
    while (node->parent->color == RBTREE_RED) {
        if (node->parent == node->parent->parent->left) {
            uncle = node->parent->parent->right;
            if (uncle->color == RBTREE_RED) {
                node->parent->color = RBTREE_BLACK;
                uncle->color = RBTREE_BLACK;
                node->parent->parent->color = RBTREE_RED;
                node = node->parent->parent;
            }
            else {
                if (node == node->parent->right) {
                    node = node->parent;
                    __rbtree_left_rotate(root, node);
                }
                node->parent->color = RBTREE_BLACK;
                node->parent->parent->color = RBTREE_RED;
                __rbtree_right_rotate(root, node->parent->parent);
            }
        }
        else {
            uncle = node->parent->parent->left;
            if (uncle->color == RBTREE_RED) {
                node->parent->color = RBTREE_BLACK;
                uncle->color = RBTREE_BLACK;
                node->parent->parent->color = RBTREE_RED;
                node = node->parent->parent;
            }
            else {
                if (node == node->parent->left) {
                    node = node->parent;
                    __rbtree_right_rotate(root, node);
                }
                node->parent->color = RBTREE_BLACK;
                node->parent->parent->color = RBTREE_RED;
                __rbtree_left_rotate(root, node->parent->parent);
            }
        }
    }
    root->root->color = RBTREE_BLACK;
}

/**
 * check current node is nil
 * @param root: rbtree node
 * @param node: current rbtree node
 * @return: 0 is nil
 */
int rbtree_is_nil(struct rbroot * const root, struct rbnode *node)
{
    return &root->nil == node ? 0 : 1;
}

/**
 * rbtree link
 * @param parent: rbtree parent node
 * @param child_ptr: rbtree parent node's child ptr var
 * @param child: rbtree newly node
 * @return: 0 is success
 */
int rbtree_link(struct rbnode *parent, struct rbnode **child_ptr, struct rbnode *child)
{
    if (parent == NULL || child_ptr == NULL || child == NULL)
        return -1;

    child->parent = parent;
    *child_ptr = child;

    return 0;
}

static void __rbtree_transplant(struct rbroot *root, struct rbnode *u, struct rbnode *v)
{
    if (u->parent == &root->nil)
        root->root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;
    v->parent = u->parent;
}

static void __rbtree_delete_fixup(struct rbroot *root, struct rbnode *node)
{
    struct rbnode *bro;
    while (node != root->root && node->color == RBTREE_BLACK) {
        if (node == node->parent->left) {
            bro = node->parent->right;
            if (bro->color == RBTREE_RED) {
                bro->color = RBTREE_BLACK;
                node->parent->color = RBTREE_RED;
                __rbtree_left_rotate(root, node->parent);
                bro = node->parent->right;
            }
            if (bro->left->color == RBTREE_BLACK && bro->right->color == RBTREE_BLACK) {
                bro->color = RBTREE_RED;
                node = node->parent;
            }
            else {
                if (bro->right->color == RBTREE_BLACK) {
                    bro->left->color = RBTREE_BLACK;
                    bro->color = RBTREE_RED;
                    __rbtree_right_rotate(root, bro);
                    bro = node->parent->right;
                }
                bro->color = node->parent->color;
                node->parent->color = RBTREE_BLACK;
                bro->right->color = RBTREE_BLACK;
                __rbtree_left_rotate(root, node->parent);
                node = root->root;
            }
        }
        else {
            bro = node->parent->left;
            if (bro->color == RBTREE_RED) {
                bro->color = RBTREE_BLACK;
                node->parent->color = RBTREE_RED;
                __rbtree_right_rotate(root, node->parent);
                bro = node->parent->left;
            }
            if (bro->left->color == RBTREE_BLACK && bro->right->color == RBTREE_BLACK) {
                bro->color = RBTREE_RED;
                node = node->parent;
            }
            else {
                if (bro->left->color == RBTREE_BLACK) {
                    bro->right->color = RBTREE_BLACK;
                    bro->color = RBTREE_RED;
                    __rbtree_left_rotate(root, bro);
                    bro = node->parent->left;
                }
                bro->color = node->parent->color;
                node->parent->color = RBTREE_BLACK;
                bro->left->color = RBTREE_BLACK;
                __rbtree_right_rotate(root, node->parent);
                node = root->root;
            }
        }
    }
}

static struct rbnode *__rbtree_minimum(struct rbroot * const root, struct rbnode *node)
{
    while (node->left != &root->nil) {
        node = node->left;
    }
    return node;
}

/**
 * rbtree delete node
 * @param root: rbtree root
 * @param node: rbtree node which will be deleted
 * @return: 0 is success
 */
int rbtree_delete(struct rbroot * const root, struct rbnode *node)
{
    struct rbnode *origin = node;
    struct rbnode *child;
    enum rbtree_color origin_color = origin->color;

    if (node->left == &root->nil) {
        child = node->right;
        __rbtree_transplant(root, node, node->right);
    }
    else if (node->right == &root->nil) {
        child = node->left;
        __rbtree_transplant(root, node, node->left);
    }
    else {
        origin = __rbtree_minimum(root, node->right);
        origin_color = origin->color;
        child = origin->right;
        if (origin->parent == node)
            child->parent = origin;
        else {
            __rbtree_transplant(root, origin, origin->right);
            origin->right = node->right;
            origin->right->parent = origin;
        }
        __rbtree_transplant(root, node, origin);
        origin->left = node->left;
        origin->left->parent = origin;
        origin->color = node->color;
    }
    if (origin_color == RBTREE_BLACK)
        __rbtree_delete_fixup(root, child);
    return 0;
}

static struct rbnode * __first(struct rbroot * const root, struct rbnode * node)
{
    while (node != &root->nil && node->left != &root->nil) {
        node = node->left;
    }
    return node;
}

struct rbnode * rbtree_first(struct rbroot * const root)
{
    return __first(root, root->root);
}

struct rbnode * rbtree_next(struct rbroot * const root, struct rbnode * node)
{
    struct rbnode * child_node;

    if (node->right != &root->nil) {
        return __first(root, node->right);
    }
    if (node != root->root) {
        if (node->parent->left == node) {
            return node->parent;
        }
        while (node != root->root && node->parent->right == node) {
            child_node = node;
            node = node->parent;
        }
        if (node->left == node) {
            return node;
        }
    }
    return &root->nil;
}
