#ifndef _GUIC_UTIL_RBTREE_H
#define _GUIC_UTIL_RBTREE_H

#include "utils/define.h"
#include <stddef.h>

enum rbtree_color {
    RBTREE_RED,
    RBTREE_BLACK
};

struct rbnode {
    enum rbtree_color color;
    struct rbnode *parent;
    struct rbnode *left;
    struct rbnode *right;
};

struct rbroot {
    struct rbnode nil;
    struct rbnode *root;
};

typedef int (*rb_cmp_fptr) (struct rbnode * const a,
                            struct rbnode * const b);

/**
 * init rbtree root
 * @param: root
 * 
 */
void rbtree_root_init(struct rbroot * const root);

/**
 * init rbtree node
 * @param root: rbtree root node
 * @param node: rbtree node
 * 
 */
void rbtree_node_init(const struct rbroot * const root,
                      struct rbnode * const node);

/**
 * fix newly rbtree node
 * @param root: rbtree root
 * @param node: newly rbtree node
 */
void rbtree_fixup(struct rbroot * const root, struct rbnode *node);

/**
 * check current node is nil
 * @param root: rbtree node
 * @param node: current rbtree node
 * @return: 0 is nil
 */
int rbtree_is_nil(struct rbroot * const root, struct rbnode *node);

/**
 * rbtree link
 * @param parent: rbtree parent node
 * @param child_ptr: rbtree parent node's child ptr var
 * @param child: rbtree newly node
 * @return: 0 is success
 */
int rbtree_link(struct rbnode *parent, struct rbnode **child_ptr, struct rbnode *child);

/**
 * rbtree delete node
 * @param root: rbtree root
 * @param node: rbtree node which will be deleted
 * @return: 0 is success
 */
int rbtree_delete(struct rbroot * const root, struct rbnode *node);

struct rbnode * rbtree_first(struct rbroot * const root);

struct rbnode * rbtree_next(struct rbroot * const root, struct rbnode * node);

#endif
