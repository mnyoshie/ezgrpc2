/**
 * pthpool.h - A C library for pollable thread pools
 * */

#ifndef EZGRPC2_THPOOL_H
#define EZGRPC2_THPOOL_H
#include <time.h>
#include "ezgrpc2_list.h"

typedef struct ezgrpc2_pthpool_result_t ezgrpc2_pthpool_result_t;

/**
 * A struct describing the result of :c:func:`pthpoll_poll()`
 */
struct ezgrpc2_pthpool_result_t {
  char is_timeout;
  time_t timeout;
  void (*ret_cleanup)(void *ret);
  void (*userdata_cleanup)(void *userdata);
  void *(*func)(void *userdata);
  void *userdata, *ret;
};

typedef struct ezgrpc2_pthpool_t ezgrpc2_pthpool_t;
struct ezgrpc2_pthpool_t;

/**
 * Initializes the thread pool.
 *
 * :param workers: The numbers of threads to be spawned in
 *                 the thread pool
 * :param flags: unused. Must be set to 0.
 * :returns:
 *     * On success, a pointer to the initialized thread pool.
 *
 *     * On failure, ``NULL``.
 *
 * Example:
 *
 * .. code-block:: C
 *
 *   ezgrpc2_pthpool_t *pool = ezgrpc2_pthpool_new(4, 0);
 */
ezgrpc2_pthpool_t *ezgrpc2_pthpool_new(int workers, int flags);

/**
 * Adds task to the thread pool.
 *
 *
 * :param pool: The thread pool.
 * :param func: The function to be called when the task is
 *              to be executed.
 * :param userdata: User-defined data to be passed to
 *                  the argument of ``func`` when the task is to be executed.
 *
 *                  .. warning::
 *                    User must exercise caution in sharing the same ``userdata`` to two
 *                    or more tasks and should set param ``userdata_cleanup`` to ``NULL`` to prevent
 *                    a possible double free when :c:func:`ezgrpc2_pthpool_free()` is called.
 * :param ret_cleanup: Cleanup handler for the value returned by ``func``
 *                     when the task is not popped out of pool via a call to
 *                     :c:func:`ezgrpc2_pthpool_poll()`.
 *
 *                     This is called when the pool is stopped via :c:func:`ezgrpc2_pthpool_destroy`
 *                     and there's executed tasks not popped via :c:func:`ezgrpc2_pthpool_poll`.
 *
 *                     If ``NULL``, no cleanup is performed.
 * :param userdata_cleanup: Cleanup handler for the parameter ``userdata`` when the task is
 *                         not popped out of pool via a call to :c:func:`ezgrpc2_pthpool_poll()`.
 *
 *                         This is called when the pool is stopped via :c:func:`ezgrpc2_pthpool_destroy`
 *                         and there's tasks left in the queue unexecuted.
 *
 *                         If ``NULL``, no cleanup is performed.
 * :returns: The :c:func:`ezgrpc2_pthpool_add_task` function returns an integer indicating the result
 *           of the operation as follows:
 *
 *           * 0, if the addition of the task is successfull.
 *
 *           * 1, if the queue is full at max of SIZE_MAX.
 *
 *           * 2, if memory cannot be allocated.
 *
 *
 * Example 1:
 *
 * .. code-block:: C
 *
 *    #include <stdlib.h> // free()
 *    #include "pthpool.h"
 *
 *    void *callback(void *data){return  NULL};
 *
 *    int main() {
 *      ezgrpc2_pthpool_t *pool = ezgrpc2_pthpool_new(4, 0);
 *
 *      //free(NULL) has no effect
 *      ezgrpc2_pthpool_add_task(pool, callback, NULL, free, free);
 *      ezgrpc2_pthpool_free(pool);
 *      return 0;
 *    }
 *
 *
 * Example 2:
 *
 * .. code-block:: C
 
 *    #include <stdlib.h>
 *    #include <unistd.h>
 *    #include "pthpool.h"
 *    void ret_cleanup(void *p) {
 *      puts("called ret_cleanup");
 *      free(p);
 *    }
 *    void userdata_cleanup(void *p) {
 *      puts("called userdata_cleanup");
 *      free(p);
 *    }
 *    void *callback(void *data){return  malloc(32);};
 *
 *    int main() {
 *      ezgrpc2_pthpool_t *pool = ezgrpc2_pthpool_new(4, 0);
 *      for (int  i = 0; i < 5; i++)
 *        ezgrpc2_pthpool_add_task(pool, callback, malloc(i), ret_cleanup, userdata_cleanup);
 *      sleep(1);
 *      // we did not poll
 *      ezgrpc2_pthpool_free(pool);
 *    }
 *
 * Result 2: :: 
 *
 *    called userdata_cleanup
 *    called ret_cleanup
 *    called userdata_cleanup
 *    called ret_cleanup
 *    called userdata_cleanup
 *    called ret_cleanup
 *    called userdata_cleanup
 *    called ret_cleanup
 *    called userdata_cleanup
 *    called ret_cleanup
 *     
 */
int ezgrpc2_pthpool_add_task(ezgrpc2_pthpool_t *pool, void *(*func)(void*), void *userdata, void (*ret_cleanup)(void *), void (*userdata_cleanup)(void*));

int ezgrpc2_pthpool_add_task2(ezgrpc2_pthpool_t *pool, void *(*func)(void*), void *userdata, void (*ret_cleanup)(void *), void (*userdata_cleanup)(void*), time_t timeout);

/**
 * Destroys the thread pool.
 *
 * :param pool: The thread pool.
 *
 * If there's unexecuted tasks left in the pool, it is popped and the 
 * registered ``userdata_cleanup``, if setted, is called.
 *
 * If there's executed tasks left in pool, not popped via, :c:func:`ezgrpc2_pthpool_poll()`,
 * it is popped, and the registered,  ``userdata_cleanup`` and ``ret_cleanup``,
 * if setted, is called with the argument ``userdata`` and ``ret``, respectively.
 *
 */
void ezgrpc2_pthpool_free(ezgrpc2_pthpool_t *pool);


/**
 * Polls for finished tasks in the thread pool amd appends it to ``ezgrpc2_list_results``.
 *
 * :param pool: The thread pool
 * :param list: The list to be filled.
 * :returns: None
 *
 * The filled ``ezgrpc2_list_results`` is freed by repeatedly popping it with
 * :c:func:`ezgrpc2_list_popb` until it returns NULL.
 *
 * The address returned by :c:func:`ezgrpc2_list_popb`, for ``list`` filled
 * by :c:func:`ezgrpc2_pthpool_poll` is of address which can be safely casted to
 * a pointer to :c:struct:`ezgrpc2_pthpool_result_t`. This :c:struct:`ezgrpc2_pthpool_result_t`
 * must be freed with :c:func:`free`.
 *
 * Example 1:
 *
 * .. code-block:: C
 *
 *    #include <stdio.h>
 *    #include <stdlib.h>
 *    #include "pthpool.h"
 *
 *    void *callback(void *data){return (char *)data + 1};
 *
 *    int main() {
 *      ezgrpc2_pthpool_t *pool = ezgrpc2_pthpool_new(4, 0);
 *      ezgrpc2_pthpool_add_task(pool, callback, (void*)0x32, NULL, NULL);
 *      ezgrpc2_pthpool_add_task(pool, callback, (void*)0x192, NULL, NULL);
 *      // wait for the tasks to get executed
 *      sleep(2);
 *      ezgrpc2_list_t l;
 *      ezgrpc2_pthpool_poll(pool, &l);
 *      ezgrpc2_pthpool_result_t *t;
 *      while ((t = ezgrpc2_list_popb(&l)) != NULL) {
 *        printf("userdata = %p\n", t->userdata);
 *        printf("ret = %p\n", t->ret);
 *        free(t);
 *      }
 *      ezgrpc2_pthpool_free(pool);
 *    }
 *
 * Result 1: :: 
 *
 *    userdata = 0x32
 *    ret = 0x33
 *    userdata = 0x192
 *    ret = 0x193
 */
void ezgrpc2_pthpool_poll(ezgrpc2_pthpool_t *pool, ezgrpc2_list_t *ezgrpc2_list_results);


/**
 * Checks if there are tasks running in the pool.
 *
 * :param pool: The thread pool
 * :returns: If empty, ``1``
 *
 *           If not empty, ``0``
 */
int ezgrpc2_pthpool_is_empty(ezgrpc2_pthpool_t *pool);

void ezgrpc2_pthpool_stop_and_join(ezgrpc2_pthpool_t *pool);
#endif
