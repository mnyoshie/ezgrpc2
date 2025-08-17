#ifndef EZGRPC2_LIST_H
#define EZGRPC2_LIST_H
#include <stdlib.h>



/**
 * An opaque list context. 
 *
 */
typedef struct ezgrpc2_list_t ezgrpc2_list_t;

/**
 *   Create a new list.
 *
 */
ezgrpc2_list_t *ezgrpc2_list_new(void *unused);

void ezgrpc2_list_free(ezgrpc2_list_t *list);


/**
 *   Counts the number of elements at the address pointed by the parameter ``list``
 *
 */
size_t ezgrpc2_list_count(ezgrpc2_list_t *list);

/**
 * Pushes the user defined ``userdata`` to the front of the list ``list``
 *
 * :param list: The list
 * :param userdata: User defined ``userdata``
 * :returns: * On success, 0
 *
 *           * On failure, non-zero
 */
int ezgrpc2_list_push_back(ezgrpc2_list_t *list, void *userdata);

/**
 * Pushes the user defined ``userdata`` to the back of the list ``list``
 *
 * :param list: The list
 * :param userdata: User defined ``userdata``
 * :returns: * On success, 0
 *
 *           * On failure, non-zero
 */
int ezgrpc2_list_push_front(ezgrpc2_list_t *list, void *userdata);

/**
 * Removes the user defined data from the last element in
 * the list, ``list``
 *
 * :param list: The list
 * :param userdata: User defined ``userdata``
 * :returns: * On success, the user defined ``userdata``
 *
 *           * On an empty list, ``NULL``.
 */
void *ezgrpc2_list_pop_front(ezgrpc2_list_t *list);

void *ezgrpc2_list_pop_back(ezgrpc2_list_t *list);

/**
 * Peeks from the back of the list, ``list``.
 *
 * :param list: The list
 * :param userdata: User defined ``userdata``
 * :returns: * On success, the user defined, ``userdata``
 *
 *           * On an empty list, ``NULL``.
 */
void *ezgrpc2_list_peek_front(ezgrpc2_list_t *list);


/**
 * Appends the list ``src`` to the list ``dst``.
 *
 * :param dst: The destination list
 * :param src: The source list
 */
void ezgrpc2_list_concat_and_empty_src(ezgrpc2_list_t *dst, ezgrpc2_list_t *src);


/**
 * The :c:func:`ezgrpc2_list_find()` function finds and returns the user defined ``userdata`` when ``cmp(userdata, cmpdata)``
 * returns 1.
 *
 * The implementation of the callback comparison function, ``cmp`` must return 1
 * in a matching ``userdata``, else 0.
 *
 * :param list: The list
 * :param cmp: The callback comparison function
 * :param cmpdata: User defined ``cmpdata`` to be passed to the comparison function.
 * :returns: * On a matching userdata, it returns the matching user defined ``userdata``
 *
 *           * Else ``NULL``.
 *
 * Example 1:
 *
 * .. code-block:: C
 *
 *    #include "list.h"
 *    
 *    int cmp(void *data, void *cmpdata) {
 *      return data == userdata;
 *    }
 *    
 *    int main() {
 *      ezgrpc2_list_t l;
 *      ezgrpc2_list_init(&l);
 *      ezgrpc2_list_push_front(&l, (void*)0x1);
 *      ezgrpc2_list_push_front(&l, (void*)0x4);
 *      ezgrpc2_list_push_front(&l, (void*)0x11);
 *    
 *      ezgrpc2_list_find(&l, cmp, (void*)4); // returns 4
 *      ezgrpc2_list_find(&l, cmp, (void*)13); // returns NULL
 *      return 0;
 *    }
 */
void *ezgrpc2_list_find(ezgrpc2_list_t *list, int (*cmp)(const void *data, const void *userdata), void *userdata);

/**
 * The :c:func:`ezgrpc2_list_remove()` function removes and returns the user defined ``userdata`` when ``cmp(userdata, cmpdata)``
 * returns 1.
 *
 * The implementation of the callback comparison function, ``cmp`` must return 1
 * in a matching ``userdata``, else 0.
 *
 * :param list: The list
 * :param cmp: The callback comparison function
 * :param cmpdata: User defined ``cmpdata`` to be passed to the comparison function.
 * :returns: * On a matching userdata, it returns the matching user defined ``userdata``
 *             and removes it from the list.
 *
 *           * Else ``NULL``.
 *
 * Example 1:
 *
 * .. code-block:: C
 *
 *    #include "list.h"
 *    
 *    int cmp(void *data, void *cmpdata) {
 *      return data == userdata;
 *    }
 *    
 *    int main() {
 *      ezgrpc2_list_t l;
 *      ezgrpc2_list_init(&l);
 *      ezgrpc2_list_push_front(&l, (void*)0x1);
 *      ezgrpc2_list_push_front(&l, (void*)0x4);
 *      ezgrpc2_list_push_front(&l, (void*)0x11);
 *    
 *      ezgrpc2_list_remove(&l, cmp, (void*)4); // returns 4
 *      ezgrpc2_list_remove(&l, cmp, (void*)13); // returns NULL
 *      return 0;
 *    }
 */
void *ezgrpc2_list_remove(ezgrpc2_list_t *list, int (*cmp)(const void *data, const void *userdata), void *userdata);

/**
 * Checks if the list is empty.
 *
 * :returns: * On an empty list, 1
 *
 *           * Else, 0
 */
int ezgrpc2_list_is_empty(ezgrpc2_list_t *list);


void ezgrpc2_list_foreach(ezgrpc2_list_t *list, void (*func)(const void *data, const void *userdata), void *userdata);
#endif
