/* pthpool.c - A pollable thread pool */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "thpool.h"

#define force_assert(s)                                                        \
  do {                                                                         \
    if (!(s)) {                                                                \
      fprintf(stderr, "%s:%d @ %s: assertion failed: " #s "\n", __FILE__,      \
              __LINE__, __func__);                                             \
      abort();                                                                 \
    }                                                                          \
  } while (0)
struct thpool_t {
  size_t max_queue, max_finished;

#ifdef NO_COUNT
  size_t nb_finished, nb_queue;
#endif
  // task queue
  list_t queue;
  // finished task queue
  list_t finished;

  int nb_threads;
  pthread_t *threads;

  pthread_mutex_t mutex;
  pthread_cond_t wcond;
  pthread_cond_t fcond;
  volatile int running;
  char stop;
};



static void *thpool_worker(void *arg) {
  thpool_t *pool = arg;

  force_assert(!pthread_mutex_lock(&pool->mutex));
  pool->running++;
  force_assert(!pthread_mutex_unlock(&pool->mutex));

  while (1) {
    force_assert(!pthread_mutex_lock(&pool->mutex));
    task_t *task;
    while (!pool->stop && (task = list_pop(&pool->queue)) == NULL)
      pthread_cond_wait(&pool->wcond, &pool->mutex);
    if (pool->stop) {
      if (task != NULL)
        list_add(&pool->queue, task);
      break;
    }

    force_assert(!pthread_mutex_unlock(&pool->mutex));
    if (task != NULL) {
      task->ret = task->func(task->userdata);

      force_assert(!pthread_mutex_lock(&pool->mutex));
#ifdef NO_COUNT
      pool->nb_queue--;
      pool->nb_finished++;
#endif
      list_add(&pool->finished, task);
      force_assert(!pthread_mutex_unlock(&pool->mutex));
    }
  }
  pool->running--;

  force_assert(!pthread_mutex_unlock(&pool->mutex));
  return NULL;
}

thpool_t *thpool_init(int workers, int flags) {
  thpool_t *pool = malloc(sizeof(*pool));
  pthread_mutex_init(&pool->mutex, NULL);
  pthread_cond_init(&pool->wcond, NULL);
  pthread_cond_init(&pool->fcond, NULL);

  list_init(&pool->queue);
  list_init(&pool->finished);
#ifdef NO_COUNT
  pool->nb_finished = 0;
  pool->nb_queue = 0;
#endif
  pool->running = 0;
  pool->nb_threads = workers;
  pool->stop = 0;

  pthread_t *threads = malloc(sizeof(*threads)*workers);
  for (int i = 0; i < workers; i++)
    pthread_create(threads + i, NULL, thpool_worker, pool);
  pool->threads = threads;

  while (pool->running != workers);
  return pool;
}

int thpool_add_task(thpool_t *pool, void *(*func)(void*), void *userdata, void (*ret_cleanup)(void *), void (*userdata_cleanup)(void*)) {
  static size_t max_queue = 0;
  force_assert(!pthread_mutex_lock(&pool->mutex));
  int c = 0;

#ifdef NO_COUNT
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
  task->func = func;
  task->userdata = userdata;
  task->ret = NULL;
  task->userdata_cleanup = userdata_cleanup;
  task->ret_cleanup = ret_cleanup;

  if (list_add(&pool->queue, task)) {
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
void thpool_poll(thpool_t *pool, list_t *l) {
  force_assert(!pthread_mutex_lock(&pool->mutex));

  *l = pool->finished;
  list_init(&pool->finished);
  //pool->nb_finished = 0;

  force_assert(!pthread_mutex_unlock(&pool->mutex));
}

void thpool_destroy(thpool_t *pool) {
  force_assert(!pthread_mutex_lock(&pool->mutex));
  pool->stop = 1;
  force_assert(!pthread_cond_broadcast(&pool->wcond));
  force_assert(!pthread_mutex_unlock(&pool->mutex));
  for (int i = 0; i < pool->nb_threads; i++)
    pthread_join(pool->threads[i], NULL);

  task_t *d;
  while ((d = list_pop(&pool->finished)) != NULL) {
    if (d->userdata_cleanup != NULL)
      d->userdata_cleanup(d->userdata);
    if (d->ret_cleanup != NULL)
      d->ret_cleanup(d->ret);
  }

  while ((d = list_pop(&pool->queue)) != NULL)
    if (d->userdata_cleanup != NULL)
      d->userdata_cleanup(d->userdata);

  free(pool->threads);
  free(pool);
}

static void *task(void *data) {
  printf("hello task %p\n",(int) data);
  return malloc(42);
}

int main0() {
#if 1
  thpool_t *thpool = thpool_init(4, -1);

  list_t finished;
  for (int i = 0; 1; i++) {
    thpool_add_task(thpool, task, malloc(32), free, free);
    thpool_poll(thpool, &finished);
    //printf("polled %zu\n", c);
    task_t *r;
    while ((r = list_pop(&finished)) != NULL) {
      free(r->userdata);
      free(r->ret);
      free(r);
    }
  }

  thpool_destroy(thpool);
#else
  list_t l;
  list_init(&l);
  list_add(&l, (void*)0x123);
  list_add(&l, (void*)0x13);
  list_add(&l, (void*)0x23);
  list_pop(&l);
  list_pop(&l);
  list_add(&l, (void*)0x125);
  list_add(&l, (void*)0x925);
  list_pop(&l);
  list_pop(&l);
  list_pop(&l);
  list_pop(&l);
  list_pop(&l);
  list_pop(&l);
  list_add(&l, (void*)0x325);

  list_print(&l);
#endif

  return 0;
}

