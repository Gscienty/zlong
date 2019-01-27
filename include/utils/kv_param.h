#ifndef _ZL_UTILS_KV_PARAM_H
#define _ZL_UTILS_KV_PARAM_H

#include "utils/rbtree.h"
#include <stdbool.h>

struct kv_param {
    struct rbnode node;
    char * key;
    char * val;
};

void zl_kv_param_init(struct kv_param * const param,
                      struct rbroot * const root);

void zl_kv_param_clear(struct kv_param * const param);

void zl_kv_param_set(struct kv_param * const param,
                     char * key,
                     char * val);

void zl_kv_param_dict_clear(struct rbroot * const root);

char * zl_kv_param_dict_find(struct rbroot * const root,
                             const char * const key);

bool zl_kv_param_dict_add(struct rbroot * const root,
                          struct kv_param * const param);

bool zl_kv_param_dict_delete(struct rbroot * const root,
                             const char * const key);

#endif
