#ifndef _ZL_UTILS_DEFINE_H
#define _ZL_UTILS_DEFINE_H

#define container_of(ptr, type, member) \
    ((type *) ((void *) ptr - (void *)(&((type *) 0)->member - 0)))

#endif
