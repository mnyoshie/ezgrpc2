/* pthpool.h - pollable thread pool */
#ifndef MNTHPOOL_H
#define MNTHPOOL_H
#include <time.h>
#include "list.h"

typedef struct task_t task_t;
struct task_t {
  char is_timeout;
  time_t timeout;
  void (*ret_cleanup)(void*), (*userdata_cleanup)(void*);
  void *(*func)(void *);
  void *userdata, *ret;
};

typedef struct pthpool_t pthpool_t;
struct pthpool_t;

/**
 * .. c:function:: pthpool_t *pthpool_init(int workers, int flags)
 *
 *    Initializes the thread pool.
 *
 *    :param workers: The numbers of threads to be spawned in
 *                    the thread pool
 *    :param flags: unused. Must be set to -1.
 *    :returns: On success, a pointer to the initialized thread pool.
 *
 *              On failure, ``NULL``.
 *
 *    Example:
 *
 *    .. code-block:: C
 *
 *      pthpool_t *pool = pthpool_init(4, -1);
 */
pthpool_t *pthpool_init(int workers, int flags);

/**
 * .. c:function:: int pthpool_add_task(pthpool_t *pool, void *(*func)(void*), void *userdata, void (*ret_cleanup)(void *), void (*userdata_cleanup)(void*))
 *
 *    Adds task to the thread pool.
 *
 *
 *    :param pool: The thread pool.
 *    :param func: The function to be called when the task is
 *                 to be executed.
 *    :param userdata: User-defined data to be passed to
 *                     the argument of ``func`` when the task is to be executed.
 *
 *                     .. warning::
 *                       User must exercise caution in sharing the same ``userdata`` to two
 *                       or more tasks and should set param ``userdata_cleanup`` to ``NULL`` to prevent
 *                       a possible double free when :c:func:`pthpool_destroy` is called.
 *    :param ret_cleanup: Cleanup handler for the value returned by ``func``
 *                        when the task is not popped out of pool via a call to
 *                        :c:func:`pthpool_poll()`.
 *
 *                        This is called when the pool is stopped via :c:func:`pthpool_destroy`
 *                        and there's executed tasks not popped via :c:func:`pthpool_poll`.
 *
 *                        If ``NULL``, no cleanup is performed.
 *    :param userdata_cleanup: Cleanup handler for the parameter ``userdata`` when the task is
 *                            not popped out of pool via a call to :c:func:`pthpool_poll()`.
 *
 *                            This is called when the pool is stopped via :c:func:`pthpool_destroy`
 *                            and there's tasks left in the queue unexecuted.
 *
 *                            If ``NULL``, no cleanup is performed.
 *    :returns: The :c:func:`pthpool_add_task` function returns an integer indicating the result
 *              of the operation as follows:
 *
 *              * 0, if the addition of the task is successfull.
 *
 *              * 1, if the queue is full at max of SIZE_MAX.
 *
 *              * 2, if memory cannot be allocated.
 *
 *
 *    Example 1:
 *
 *    .. code-block:: C
 *
 *       void *callback(void *data){return  NULL};
 *
 *       int main() {
 *         pthpool_t *pool = pthpool_init(4, -1, -1);
 *         pthpool_add_task(pool, callback, NULL, NULL, NULL);
 *       }
 *
 *
 *    Example 2:
 *
 *    .. code-block:: C
 
 *       #include <stdlib.h>
 *       #include <unistd.h>
 *       #include "pthpool.h"
 *       void ret_cleanup(void *p) {
 *         puts("called ret_cleanup");
 *         free(p);
 *       }
 *       void userdata_cleanup(void *p) {
 *         puts("called userdata_cleanup");
 *         free(p);
 *       }
 *       void *callback(void *data){return  malloc(32);};
 *
 *       int main() {
 *         pthpool_t *pool = pthpool_init(4, -1,);
 *         for (int  i = 0; i < 5; i++)
 *           pthpool_add_task(pool, callback, malloc(i), ret_cleanup, userdata_cleanup);
 *         sleep(1);
 *         pthpool_destroy(pool);
 *       }
 *
 *    Result 2: :: 
 *
 *       called userdata_cleanup
 *       called ret_cleanup
 *       called userdata_cleanup
 *       called ret_cleanup
 *       called userdata_cleanup
 *       called ret_cleanup
 *       called userdata_cleanup
 *       called ret_cleanup
 *       called userdata_cleanup
 *       called ret_cleanup
 *     
 */
int pthpool_add_task(pthpool_t *pool, void *(*func)(void*), void *userdata, void (*ret_cleanup)(void *), void (*userdata_cleanup)(void*));

int pthpool_add_task2(pthpool_t *pool, void *(*func)(void*), void *userdata, void (*ret_cleanup)(void *), void (*userdata_cleanup)(void*), time_t timeout);

/**
 * .. c:function:: void pthpool_destroy(pthpool_t *pool)
 *
 *    Destroys the thread pool.
 *
 *    :param pool: The thread pool.
 *
 *    If there's left unexecuted tasks in the pool, it is popped and the 
 *    registered ``userdata_cleanup``, if setted, is called.
 *
 *    If there's left executed tasks in pool, not popped via, :c:func:pthpool_poll,
 *    it is popped, and the registered,  ``userdata_cleanup`` and ``ret_cleanup``,
 *    if setted, is called with the argument ``userdata`` and ``ret``, respectively.
 *
 */
void pthpool_destroy(pthpool_t *pool);


/**
 * .. c:function:: list_t *pthpool_poll(pthpool_t *pool, list_t *list)
 *
 *    Polls for finished tasks in the thread pool.
 *
 *    :param pool: The thread pool
 *    :param list: The list to be filled.
 *    :returns: The list of finished tasks.
 *
 *    The filled ``list`` is freed by repeatedly popping it with
 *    :c:func:`list_pop` until it returns NULL.
 *
 *    The address returned by :c:func:`list_pop`, for ``list`` filled
 *    by :c:func:`pthpool_poll` is of address which can be safely casted to
 *    :c:struct:`task_t *`. This `task_t *` must be freed with
 *    :c:func:`free`.
 *
 *    Example 1:
 *
 *    .. code-block:: C
 *
 *       #include <stdio.h>
 *       #include <stdlib.h>
 *       #include "pthpool.h"
 *
 *       void *callback(void *data){return (char *)data + 1};
 *
 *       int main() {
 *         pthpool_t *pool = pthpool_init(4, -1, -1);
 *         pthpool_add_task(pool, callback, (void*)0x32, NULL, NULL);
 *         pthpool_add_task(pool, callback, (void*)0x192, NULL, NULL);
 *         // wait for the tasks to get executed
 *         sleep(2);
 *         list_t l;
 *         pthpool_poll(pool, &l);
 *         task_t *t;
 *         while ((t = list_pop(&l)) != NULL) {
 *           printf("userdata = %p\n", t->userdata);
 *           printf("ret = %p\n", t->ret);
 *           free(t);
 *         }
 *         pthpool_destroy(pool);
 *       }
 *
 *    Result 1: :: 
 *
 *       userdata = 0x32
 *       ret = 0x33
 *       userdata = 0x192
 *       ret = 0x193
 *
 *
 */
void pthpool_poll(pthpool_t *pool, list_t *list_tasks);


int pthpool_is_empty(pthpool_t *pool);

#endif