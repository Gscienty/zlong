#include "utils/linked_list.h"

/**
 * init linked list head
 * @param head: linked list head
 * 
 */
void ll_head_init(struct llnode * const head)
{
    head->next = head;
    head->prev = head;
}

/**
 * check linked list is empty
 * @param head: linked list
 * @return: is empty
 * 
 */
int ll_empty(const struct llnode * const head)
{
    return head == head->next;
}


/**
 * insert entity before someone
 * @param place: insert place
 * @param newly: new entity
 * 
 */
void ll_insert_before(struct llnode * const place,
                      struct llnode * const newly)
{
    newly->next       = place;
    newly->prev       = place->prev;
    place->prev->next = newly;
    place->prev       = newly;
}

/**
 * insert entity after someone
 * @param place: insert place
 * @param newly: new entity
 * 
 */
void ll_insert_after(struct llnode * const place,
                     struct llnode * const newly)
{
    newly->prev       = place;
    newly->next       = place->next;
    place->next->prev = newly;
    place->next       = newly;
}

/**
 * remove entity
 * @param : removed node
 *
 */
void ll_remove(struct llnode * const node)
{
    struct llnode *prev = node->prev;
    struct llnode *next = node->next;

    prev->next = node->next;
    next->prev = node->prev;
}
