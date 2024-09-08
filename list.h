#ifndef MNLIST_H
#define MNLIST_H
#include <stdlib.h>

typedef struct list_t list_t;

/**
 * An opaque list context
 */
struct list_t {
  void *head, **tail;
};

/**
 *   Initializes the list at the address pointed by the parameter ``list``
 *
 */
void list_init(list_t *list);


/**
 *   Counts the number of list at the address pointed by the parameter ``list``
 *
 */
size_t list_count(list_t *list);

/**
 * Pushes the user defined ``userdata`` to the front of the list ``list``
 *
 * :param list: The list
 * :param userdata: User defined ``userdata``
 * :returns: * On success, 0
 *
 *           * On failure, non-zero
 */
int list_pushf(list_t *list, void *userdata);

/**
 * Pushes the user defined ``userdata`` to the back of the list ``list``
 *
 * :param list: The list
 * :param userdata: User defined ``userdata``
 * :returns: * On success, 0
 *
 *           * On failure, non-zero
 */
int list_pushb(list_t *list, void *userdata);

/**
 * Pops the user defined ``userdata`` from the back of the list, ``list``
 *
 * :param list: The list
 * :param userdata: User defined ``userdata``
 * :returns: * On success, the user defined ``userdata``
 *
 *           * On an empty list, ``NULL``.
 */
void *list_popb(list_t *list);

/**
 * Peeks from the back of the list, ``list``.
 *
 * :param list: The list
 * :param userdata: User defined ``userdata``
 * :returns: * On success, the user defined, ``userdata``
 *
 *           * On an empty list, ``NULL``.
 */
void *list_peekb(list_t *list);


/**
 * Appends the list ``src`` to the list ``dst``.
 *
 * :param dst: The destination list
 * :param src: The source list
 */
void list_pushf_list(list_t *dst, list_t *src);

void list_print(list_t *);

/**
 * The :c:func:`list_find()` function finds and returns the user defined ``userdata`` when ``cmp(userdata, cmpdata)``
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
 *      list_t l;
 *      list_init(&l);
 *      list_pushf(&l, (void*)0x1);
 *      list_pushf(&l, (void*)0x4);
 *      list_pushf(&l, (void*)0x11);
 *    
 *      list_find(&l, cmp, (void*)4); // returns 4
 *      list_find(&l, cmp, (void*)13); // returns NULL
 *      return 0;
 *    }
 */
void *list_find(list_t *list, int (*cmp)(void *userdata, void *cmpdata), void *cmpdata);

/**
 * The :c:func:`list_remove()` function removes and returns the user defined ``userdata`` when ``cmp(userdata, cmpdata)``
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
 *      list_t l;
 *      list_init(&l);
 *      list_pushf(&l, (void*)0x1);
 *      list_pushf(&l, (void*)0x4);
 *      list_pushf(&l, (void*)0x11);
 *    
 *      list_remove(&l, cmp, (void*)4); // returns 4
 *      list_remove(&l, cmp, (void*)13); // returns NULL
 *      return 0;
 *    }
 */
void *list_remove(list_t *list, int (*cmp)(void *userdata, void *cmpdata), void *cmpdata);

/**
 * Checks if the list is empty.
 *
 * :returns: * On an empty list, 1
 *
 *           * Else, 0
 */
int list_is_empty(list_t *list);

#endif
