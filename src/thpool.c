/* pthpool.c - A pollable thread pool */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "ezgrpc2_list.h"
//#include "core.h"

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
  ezgrpc2_list_t *queue;
  // finished task queue

  int nb_threads;
  pthread_t *threads;

  pthread_mutex_t mutex;
  pthread_cond_t wcond;
  volatile int running;
  volatile int live;
  char stop;
};

typedef struct task_t task_t;
struct task_t {
  void (*userdata_cleanup)(void *userdata);
  void (*func)(void *userdata);
  void *userdata;
};



static void *worker(void *arg) {
  thpool_t *pool = arg;

  force_assert(!pthread_mutex_lock(&pool->mutex));
  pool->live++;
  force_assert(!pthread_mutex_unlock(&pool->mutex));

  while (1) {
    force_assert(!pthread_mutex_lock(&pool->mutex));
    task_t *task = NULL;
    while (!pool->stop && (task = ezgrpc2_list_pop_front(pool->queue)) == NULL)
      pthread_cond_wait(&pool->wcond, &pool->mutex);
    if (pool->stop) {
      if (task != NULL)
        ezgrpc2_list_push_back(pool->queue, task);
      break;
    }

    if (task != NULL)
      pool->running++;

    force_assert(!pthread_mutex_unlock(&pool->mutex));
    if (task != NULL) {
      task->func(task->userdata);
      free(task);
      force_assert(!pthread_mutex_lock(&pool->mutex));
      pool->running--;
      force_assert(!pthread_mutex_unlock(&pool->mutex));
    }
  }
  pool->live--;

  force_assert(!pthread_mutex_unlock(&pool->mutex));
  return NULL;
}

thpool_t *thpool_new(int workers, int flags) {
  thpool_t *pool = malloc(sizeof(*pool));
  pthread_mutex_init(&pool->mutex, NULL);
  pthread_cond_init(&pool->wcond, NULL);

  pool->queue = ezgrpc2_list_new(NULL);
  pool->running = 0;
  pool->live = 0;
  pool->nb_threads = workers;
  pool->stop = 0;

  pthread_t *threads = malloc(sizeof(*threads)*workers);
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  for (int i = 0; i < workers; i++)
    pthread_create(threads + i, NULL, worker, pool);
  pthread_attr_destroy(&attr);
  pool->threads = threads;

  while (pool->live != workers);
  return pool;
}

int thpool_is_empty(thpool_t *pool) {
  force_assert(!pthread_mutex_lock(&pool->mutex));
  int ret = pool->running == 0 &&
    ezgrpc2_list_peek_front(pool->queue) == NULL;

  force_assert(!pthread_mutex_unlock(&pool->mutex));
  return ret;
}

int thpool_add_task(thpool_t *pool, void (*func)(void*), void *userdata, void (*userdata_cleanup)(void*)) {
  force_assert(!pthread_mutex_lock(&pool->mutex));
  int c = 0;

  task_t *task = malloc(sizeof(*task));
  if (task == NULL) {
    c = 1;
    goto unlock;
  }
  task->func = func;
  task->userdata = userdata;
  task->userdata_cleanup = userdata_cleanup;

  if (ezgrpc2_list_push_back(pool->queue, task)) {
    c = 1;
    free(task);
    goto unlock;
  }

  force_assert(!pthread_cond_broadcast(&pool->wcond));

unlock:
  force_assert(!pthread_mutex_unlock(&pool->mutex));
  return c;
}

void thpool_stop_and_join(thpool_t *pool) {
  force_assert(!pthread_mutex_lock(&pool->mutex));
  pool->stop = 1;
  force_assert(!pthread_cond_broadcast(&pool->wcond));
  force_assert(!pthread_mutex_unlock(&pool->mutex));
  for (int i = 0; i < pool->nb_threads; i++) 
    pthread_join(pool->threads[i], NULL);

}

void thpool_free(thpool_t *pool) {
  force_assert(!pthread_mutex_lock(&pool->mutex));
  pool->stop = 1;
  force_assert(!pthread_cond_broadcast(&pool->wcond));
  force_assert(!pthread_mutex_unlock(&pool->mutex));
  for (int i = 0; i < pool->nb_threads; i++)
    pthread_join(pool->threads[i], NULL);

  task_t *d;

  while ((d = ezgrpc2_list_pop_front(pool->queue)) != NULL)
    if (d->userdata_cleanup != NULL)
      d->userdata_cleanup(d->userdata);

  free(pool->threads);
  free(pool);
}
