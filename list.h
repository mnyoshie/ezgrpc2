#ifndef MNLIST_H
#define MNLIST_H
#include <stdlib.h>

typedef struct ezgrpc2_list_t ezgrpc2_list_t;

/**
 * An opaque list context. 
 *
 * Declaration of this type such as ``ezgrpc2_list_t l``,
 * must be immediately initialized by a call to, :c:func:`ezgrpc2_list_init()` as
 * ``ezgrpc2_list_init(&l)``.
 *
 */
struct ezgrpc2_list_t {
  void *head, **tail;
};

/**
 *   Initializes the list at the address pointed by the parameter ``list``
 *
 */
void ezgrpc2_list_init(ezgrpc2_list_t *list);


/**
 *   Counts the number of list at the address pointed by the parameter ``list``
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
int ezgrpc2_list_pushf(ezgrpc2_list_t *list, void *userdata);

/**
 * Pushes the user defined ``userdata`` to the back of the list ``list``
 *
 * :param list: The list
 * :param userdata: User defined ``userdata``
 * :returns: * On success, 0
 *
 *           * On failure, non-zero
 */
int ezgrpc2_list_pushb(ezgrpc2_list_t *list, void *userdata);

/**
 * Pops the user defined ``userdata`` from the back of the list, ``list``
 *
 * :param list: The list
 * :param userdata: User defined ``userdata``
 * :returns: * On success, the user defined ``userdata``
 *
 *           * On an empty list, ``NULL``.
 */
void *ezgrpc2_list_popb(ezgrpc2_list_t *list);

/**
 * Peeks from the back of the list, ``list``.
 *
 * :param list: The list
 * :param userdata: User defined ``userdata``
 * :returns: * On success, the user defined, ``userdata``
 *
 *           * On an empty list, ``NULL``.
 */
void *ezgrpc2_list_peekb(ezgrpc2_list_t *list);


/**
 * Appends the list ``src`` to the list ``dst``.
 *
 * :param dst: The destination list
 * :param src: The source list
 */
void ezgrpc2_list_pushf_list(ezgrpc2_list_t *dst, ezgrpc2_list_t *src);

void ezgrpc2_list_print(ezgrpc2_list_t *);

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
 *      ezgrpc2_list_pushf(&l, (void*)0x1);
 *      ezgrpc2_list_pushf(&l, (void*)0x4);
 *      ezgrpc2_list_pushf(&l, (void*)0x11);
 *    
 *      ezgrpc2_list_find(&l, cmp, (void*)4); // returns 4
 *      ezgrpc2_list_find(&l, cmp, (void*)13); // returns NULL
 *      return 0;
 *    }
 */
void *ezgrpc2_list_find(ezgrpc2_list_t *list, int (*cmp)(void *userdata, void *cmpdata), void *cmpdata);

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
 *      ezgrpc2_list_pushf(&l, (void*)0x1);
 *      ezgrpc2_list_pushf(&l, (void*)0x4);
 *      ezgrpc2_list_pushf(&l, (void*)0x11);
 *    
 *      ezgrpc2_list_remove(&l, cmp, (void*)4); // returns 4
 *      ezgrpc2_list_remove(&l, cmp, (void*)13); // returns NULL
 *      return 0;
 *    }
 */
void *ezgrpc2_list_remove(ezgrpc2_list_t *list, int (*cmp)(void *userdata, void *cmpdata), void *cmpdata);

/**
 * Checks if the list is empty.
 *
 * :returns: * On an empty list, 1
 *
 *           * Else, 0
 */
int ezgrpc2_list_is_empty(ezgrpc2_list_t *list);

#endif
