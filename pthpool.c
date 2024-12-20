/* pthpool.c - A pollable thread pool */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct ezgrpc2_pthpool_result_t task_t;
#include "pthpool.h"

#define force_assert(s)                                                        \
  do {                                                                         \
    if (!(s)) {                                                                \
      fprintf(stderr, "%s:%d @ %s: assertion failed: " #s "\n", __FILE__,      \
              __LINE__, __func__);                                             \
      abort();                                                                 \
    }                                                                          \
  } while (0)

struct ezgrpc2_pthpool_t {
  size_t max_queue, max_finished;

#ifdef NO_COUNT
  size_t nb_finished, nb_queue;
#endif
  // task queue
  ezgrpc2_list_t queue;
  // finished task queue
  ezgrpc2_list_t finished;

  int nb_threads;
  pthread_t *threads;

  pthread_mutex_t mutex;
  pthread_cond_t wcond;
  volatile int running;
  volatile int live;
  char stop;
};



static void *ezgrpc2_pthpool_worker(void *arg) {
  ezgrpc2_pthpool_t *pool = arg;

  force_assert(!pthread_mutex_lock(&pool->mutex));
  pool->live++;
  force_assert(!pthread_mutex_unlock(&pool->mutex));

  while (1) {
    force_assert(!pthread_mutex_lock(&pool->mutex));
    task_t *task = NULL;
    while (!pool->stop && (task = ezgrpc2_list_popb(&pool->queue)) == NULL)
      pthread_cond_wait(&pool->wcond, &pool->mutex);
    if (pool->stop) {
      if (task != NULL)
        ezgrpc2_list_pushf(&pool->queue, task);
      break;
    }

    if (task != NULL)
      pool->running++;

    force_assert(!pthread_mutex_unlock(&pool->mutex));
    if (task != NULL) {
      if (task->timeout && time(NULL) > task->timeout)
        task->is_timeout = 1;
      else 
        task->ret = task->func(task->userdata);

      force_assert(!pthread_mutex_lock(&pool->mutex));
#ifdef NO_COUNT
      pool->nb_queue--;
      pool->nb_finished++;
#endif
      ezgrpc2_list_pushf(&pool->finished, task);
      pool->running--;
      force_assert(!pthread_mutex_unlock(&pool->mutex));
    }
  }
  pool->live--;

  force_assert(!pthread_mutex_unlock(&pool->mutex));
  return NULL;
}

ezgrpc2_pthpool_t *ezgrpc2_pthpool_init(int workers, int flags) {
  ezgrpc2_pthpool_t *pool = malloc(sizeof(*pool));
  pthread_mutex_init(&pool->mutex, NULL);
  pthread_cond_init(&pool->wcond, NULL);

  ezgrpc2_list_init(&pool->queue);
  ezgrpc2_list_init(&pool->finished);
#ifdef NO_COUNT
  pool->nb_finished = 0;
  pool->nb_queue = 0;
#endif
  pool->running = 0;
  pool->live = 0;
  pool->nb_threads = workers;
  pool->stop = 0;

  pthread_t *threads = malloc(sizeof(*threads)*workers);
  for (int i = 0; i < workers; i++)
    pthread_create(threads + i, NULL, ezgrpc2_pthpool_worker, pool);
  pool->threads = threads;

  while (pool->live != workers);
  return pool;
}

int ezgrpc2_pthpool_is_empty(ezgrpc2_pthpool_t *pool) {
  force_assert(!pthread_mutex_lock(&pool->mutex));
  int ret = pool->running == 0 &&
    ezgrpc2_list_peekb(&pool->finished) == NULL &&
    ezgrpc2_list_peekb(&pool->queue) == NULL;

  force_assert(!pthread_mutex_unlock(&pool->mutex));
  return ret;
}

int ezgrpc2_pthpool_add_task(ezgrpc2_pthpool_t *pool, void *(*func)(void*), void *userdata, void (*ret_cleanup)(void *), void (*userdata_cleanup)(void*)) {
  return ezgrpc2_pthpool_add_task2(pool, func, userdata, ret_cleanup, userdata_cleanup, 0);
}

int ezgrpc2_pthpool_add_task2(ezgrpc2_pthpool_t *pool, void *(*func)(void*), void *userdata, void (*ret_cleanup)(void *), void (*userdata_cleanup)(void*), time_t timeout) {
  force_assert(!pthread_mutex_lock(&pool->mutex));
  int c = 0;

#ifdef NO_COUNT
  static size_t max_queue = 0;
  if (pool->nb_queue > SIZE_MAX - 1) {
    c = 2;
    goto unlock;
  }
#endif

  task_t *task = malloc(sizeof(*task));
  if (task == NULL) {
    c = 1;
    goto unlock;
  }
  task->is_timeout = 0;
  task->timeout = timeout;
  task->func = func;
  task->userdata = userdata;
  task->ret = NULL;
  task->userdata_cleanup = userdata_cleanup;
  task->ret_cleanup = ret_cleanup;

  if (ezgrpc2_list_pushf(&pool->queue, task)) {
    c = 1;
    free(task);
    goto unlock;
  }
#ifdef NO_COUNT
  pool->nb_queue++;

  int p;
  if ((p = pool->nb_queue) > max_queue)
    printf("max quue %zu\n", (max_queue = p));
#endif

  force_assert(!pthread_cond_broadcast(&pool->wcond));

unlock:
  force_assert(!pthread_mutex_unlock(&pool->mutex));
  return c;
}

/* checks if there's any finished tasks in the pool.
 * returns immediately
 */
void ezgrpc2_pthpool_poll(ezgrpc2_pthpool_t *pool, ezgrpc2_list_t *l) {
  force_assert(!pthread_mutex_lock(&pool->mutex));

  //*l = pool->finished;
  ezgrpc2_list_pushf_list(l, &pool->finished);
  ezgrpc2_list_init(&pool->finished);
  //pool->nb_finished = 0;

  force_assert(!pthread_mutex_unlock(&pool->mutex));
}

void ezgrpc2_pthpool_destroy(ezgrpc2_pthpool_t *pool) {
  force_assert(!pthread_mutex_lock(&pool->mutex));
  pool->stop = 1;
  force_assert(!pthread_cond_broadcast(&pool->wcond));
  force_assert(!pthread_mutex_unlock(&pool->mutex));
  for (int i = 0; i < pool->nb_threads; i++)
    pthread_join(pool->threads[i], NULL);

  task_t *d;
  while ((d = ezgrpc2_list_popb(&pool->finished)) != NULL) {
    if (d->userdata_cleanup != NULL)
      d->userdata_cleanup(d->userdata);
    if (d->ret_cleanup != NULL)
      d->ret_cleanup(d->ret);
  }

  while ((d = ezgrpc2_list_popb(&pool->queue)) != NULL)
    if (d->userdata_cleanup != NULL)
      d->userdata_cleanup(d->userdata);

  free(pool->threads);
  free(pool);
}

#if 0
static void *task(void *data) {
  printf("hello task %p\n",(int) data);
  return malloc(42);
}

int main0() {
  ezgrpc2_pthpool_t *pthpool = ezgrpc2_pthpool_init(4, -1);

  ezgrpc2_list_t finished;
  for (int i = 0; 1; i++) {
    ezgrpc2_pthpool_add_task(pthpool, task, malloc(32), free, free);
    ezgrpc2_pthpool_poll(pthpool, &finished);
    //printf("polled %zu\n", c);
    task_t *r;
    while ((r = ezgrpc2_list_popb(&finished)) != NULL) {
      free(r->userdata);
      free(r->ret);
      free(r);
    }
  }

  ezgrpc2_pthpool_destroy(pthpool);
  ezgrpc2_list_t l;
  ezgrpc2_list_init(&l);
  ezgrpc2_list_pushf(&l, (void*)0x123);
  ezgrpc2_list_pushf(&l, (void*)0x13);
  ezgrpc2_list_pushf(&l, (void*)0x23);
  ezgrpc2_list_popb(&l);
  ezgrpc2_list_popb(&l);
  ezgrpc2_list_pushf(&l, (void*)0x125);
  ezgrpc2_list_pushf(&l, (void*)0x925);
  ezgrpc2_list_popb(&l);
  ezgrpc2_list_popb(&l);
  ezgrpc2_list_popb(&l);
  ezgrpc2_list_popb(&l);
  ezgrpc2_list_popb(&l);
  ezgrpc2_list_popb(&l);
  ezgrpc2_list_pushf(&l, (void*)0x325);

  ezgrpc2_list_print(&l);

  return 0;
}
#endif

