/**
 * thpool.h - A C library for thread pools
 * */

#ifndef THPOOL_H
#define THPOOL_H
#include <time.h>
#include "ezgrpc2_list.h"

typedef struct thpool_t thpool_t;
struct thpool_t;

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
 *   thpool_t *pool = thpool_new(4, 0);
 */
thpool_t *thpool_new(int workers, int flags);
int thpool_init(thpool_t *pool, int workers, int flags);

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
 *                    a possible double free when :c:func:`thpool_free()` is called.
 * :param ret_cleanup: Cleanup handler for the value returned by ``func``
 *                     when the task is not popped out of pool via a call to
 *                     :c:func:`thpool_poll()`.
 *
 *                     This is called when the pool is stopped via :c:func:`thpool_destroy`
 *                     and there's executed tasks not popped via :c:func:`thpool_poll`.
 *
 *                     If ``NULL``, no cleanup is performed.
 * :param userdata_cleanup: Cleanup handler for the parameter ``userdata`` when the task is
 *                         not popped out of pool via a call to :c:func:`thpool_poll()`.
 *
 *                         This is called when the pool is stopped via :c:func:`thpool_destroy`
 *                         and there's tasks left in the queue unexecuted.
 *
 *                         If ``NULL``, no cleanup is performed.
 * :returns: The :c:func:`thpool_add_task` function returns an integer indicating the result
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
 *    #include "thpool.h"
 *
 *    void *callback(void *data){return  NULL};
 *
 *    int main() {
 *      thpool_t *pool = thpool_new(4, 0);
 *
 *      //free(NULL) has no effect
 *      thpool_add_task(pool, callback, NULL, free, free);
 *      thpool_free(pool);
 *      return 0;
 *    }
 *
 *
 * Example 2:
 *
 * .. code-block:: C
 
 *    #include <stdlib.h>
 *    #include <unistd.h>
 *    #include "thpool.h"
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
 *      thpool_t *pool = thpool_new(4, 0);
 *      for (int  i = 0; i < 5; i++)
 *        thpool_add_task(pool, callback, malloc(i), ret_cleanup, userdata_cleanup);
 *      sleep(1);
 *      // we did not poll
 *      thpool_free(pool);
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
int thpool_add_task(thpool_t *pool, void (*func)(void*), void *userdata, void (*userdata_cleanup)(void*));


/**
 * Destroys the thread pool.
 *
 * :param pool: The thread pool.
 *
 * If there's unexecuted tasks left in the pool, it is popped and the 
 * registered ``userdata_cleanup``, if setted, is called.
 *
 * If there's executed tasks left in pool, not popped via, :c:func:`thpool_poll()`,
 * it is popped, and the registered,  ``userdata_cleanup`` and ``ret_cleanup``,
 * if setted, is called with the argument ``userdata`` and ``ret``, respectively.
 *
 */
void thpool_free(thpool_t pool);
void thpool_freep(thpool_t *pool);


/**
 * Checks if there are tasks running in the pool.
 *
 * :param pool: The thread pool
 * :returns: If empty, ``1``
 *
 *           If not empty, ``0``
 */
int thpool_is_empty(thpool_t *pool);

void thpool_stop_and_join(thpool_t *pool);



#endif
