#include "debug/console.h"
#include "utils/kv_param.h"
#include <string.h>
#include <malloc.h>

void zl_kv_param_init(struct kv_param * const param,
                      struct rbroot * const root)
{
    rbtree_node_init(root, &param->node);
    param->key = NULL;
    param->val = NULL;
}

void zl_kv_param_clear(struct kv_param * const param)
{
    if (param->key != NULL) {
        free(param->key);
    }
    if (param->val != NULL) {
        free(param->val);
    }
}

void zl_kv_param_set(struct kv_param * const param,
                     char * key,
                     char * val)
{
    param->key = key;
    param->val = val;
}

void zl_kv_param_dict_clear(struct rbroot * const root)
{
    struct rbnode * node;
    struct kv_param * param;
    node = root->root;

    while (node != &root->nil) {
        param = container_of(node, struct kv_param, node);

        zl_kv_param_clear(param);
        rbtree_delete(root, node);
        free(param);

        node = root->root;
    }
}

static int __param_key_cmp(const char * const a, const char * const b)
{
    return strcmp(a, b);
}

char * zl_kv_param_dict_find(struct rbroot * const root,
                             const char * const key)
{
    struct kv_param * param;
    struct rbnode * node;
    int cmpret;

    node = root->root;
    while (node != &root->nil)
    {
        param = container_of(node, struct kv_param, node);
        cmpret = __param_key_cmp(key, param->key);
        if (cmpret == 0)
            return param->val;
        else if (cmpret < 0)
            node = node->left;
        else
            node = node->right;
    }

    return NULL;
}

bool zl_kv_param_dict_add(struct rbroot * const root,
                          struct kv_param * const param)
{
    struct kv_param *parent;
    struct rbnode **place;
    int cmpret;

    if (root->root == &root->nil)
        root->root = &param->node;
    else {
        place = &root->root;

        while (*place != &root->nil) {
            parent = container_of(*place, struct kv_param, node);
            cmpret = __param_key_cmp(param->key, parent->key);
            if (cmpret == 0)
                return false;
            else if (cmpret < 0)
                place = &parent->node.left;
            else
                place = &parent->node.right;
        }

        rbtree_link(&parent->node, place, &param->node);
    }

    rbtree_fixup(root, &param->node);
    return true;
}
