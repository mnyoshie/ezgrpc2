#ifndef THPOOL_STRUCT_H
#define THPOOL_STRUCT_H

#include <pthread.h>
#include "ezgrpc2_list.h"
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
  volatile int live;
  char stop;
};
#endif
