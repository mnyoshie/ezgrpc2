/* The author disclaims copyright to this source code
 * and releases it into the public domain.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#ifdef HAVE_SECCOMP
#include <seccomp.h>
#endif

#include "ezgrpc2.h"
#include "pthpool.h"

struct userdata_t {
  i32 stream_id;
  char session_uuid[EZGRPC2_SESSION_UUID_LEN];
  char end_stream;
  int status;
  list_t list_imessages, list_omessages;
};

struct path_userdata_t {
  char is_unary;
  void *(*callback)(void*);
};


struct userdata_t *create_userdata(char session_uuid[EZGRPC2_SESSION_UUID_LEN], int32_t stream_id,
    list_t list_messages, char end_stream, int status) {
  struct userdata_t *data = malloc(sizeof(*data));
  memcpy(data->session_uuid, session_uuid, EZGRPC2_SESSION_UUID_LEN);
  data->stream_id = stream_id;
  data->list_imessages = list_messages;
  data->end_stream = end_stream;
  data->status = status;
  return data;
}

_Atomic int pp = 0;
/* No ezgrpc2_* functions must be called in this callback */
void *callback_path0(void *data){
  printf("task executed path0!!\n");
  struct userdata_t *usrdata = data;
  list_init(&usrdata->list_omessages);
  if (usrdata->status)
    return usrdata;

  /* pretend this is our response (output messages) */
  for (int i = 0; i < 5; i++, pp++) {
    ezgrpc2_message_t *msg = malloc(sizeof(*msg));
    msg->is_compressed = 0;
    msg->data = strdup("    Hello world from path0!!");
    
    msg->len = strlen(msg->data) + 1;
    *(uint32_t*)(msg->data) = pp;
    list_pushf(&usrdata->list_omessages, msg);
  }


  /* cleanup (input messages) */
  list_t *list_imessages = &usrdata->list_imessages;
  {
    ezgrpc2_message_t *msg;
    while ((msg = list_popb(list_imessages)) != NULL) {
      free(msg->data);
      free(msg);
    }
  }

  return data;
}

/* No ezgrpc2_* functions must be called in this callback */
void *callback_path1(void *data){
  printf("task executed path1!!\n");
  struct userdata_t *usrdata = data;
  list_init(&usrdata->list_omessages);
  if (usrdata->status)
    return usrdata;

  /* pretend this is our response (output messages) */
  for (int i = 0; i < 5; i++, pp++) {
    ezgrpc2_message_t *msg = malloc(sizeof(*msg));
    msg->is_compressed = 0;
    msg->data = strdup("    Hello world from path1!!");
    
    msg->len = strlen(msg->data) + 1;
    *(uint32_t*)(msg->data) = pp;
    list_pushf(&usrdata->list_omessages, msg);
  }


  /* cleanup (input messages) */
  list_t *list_imessages = &usrdata->list_imessages;
  {
    ezgrpc2_message_t *msg;
    while ((msg = list_popb(list_imessages)) != NULL) {
      free(msg->data);
      free(msg);
    }
  }

  return data;
}

void free_list_messages(list_t *list_messages){
  ezgrpc2_message_t *msg;
  while ((msg = list_popb(list_messages)) != NULL) {
    free(msg->data);
    free(msg);
  }
}

static void handle_events(ezgrpc2_server_t *server, ezgrpc2_path_t *paths, size_t nb_paths,
    pthpool_t *opool, pthpool_t *upool) {
  ezgrpc2_event_t *event;
  struct userdata_t *data = NULL;
  struct path_userdata_t *path_userdata = NULL;
  /* check for events in the paths */
  for (size_t i = 0; i < nb_paths; i++) {
    path_userdata = paths[i].userdata;
    while ((event = list_popb(&paths[i].list_events)) != NULL) {
      switch(event->type) {
        case EZGRPC2_EVENT_MESSAGE:
          printf("id %d, event message %zu end stream %d\n", event->message.stream_id,
              list_count(&event->message.list_messages), event->message.end_stream);

          if (path_userdata->is_unary) {
            if (event->message.end_stream == 0 || list_count(&event->message.list_messages) != 1) {
              ezgrpc2_session_end_stream(server, event->session_uuid, event->message.stream_id, EZGRPC2_STATUS_INVALID_ARGUMENT);

              free_list_messages(&event->message.list_messages);
              break;
            }
          }
#if 1
          data = create_userdata(
              event->session_uuid, event->message.stream_id,
              event->message.list_messages,
              event->message.end_stream, 0);
          /* we've taken ownership of this. clear */
          list_init(&event->message.list_messages);

          /* add task to pool */
          /* if path/service is unary, add task to upool, else add to opool */
          if (path_userdata->is_unary)
            pthpool_add_task(upool, path_userdata->callback, data, NULL, NULL);
          else
            pthpool_add_task(opool, path_userdata->callback, data, NULL, NULL);
#else
          ezgrpc2_session_end_session(server, event->session_uuid, 0, 0);
#endif
          break;
        case EZGRPC2_EVENT_DATALOSS:
          printf("id %d, event dataloss\n", event->dataloss.stream_id);
          /* Client ended the stream with a truncated gRPC message.
           *
           * If previously, we got a EZGRPC2_EVENT_MESSAGE in this stream id,
           * that messages are valid. But event->message.end_stream is not set.
           *
           * In place of that, we'll be receving a EZGRPC2_EVENT_DATALOSS.
           *
           * You can handle this the same as EZGRPC2_EVENT_MESSAGE except
           * there are no messages to process.
           *
           * We will not be getting a EZGRPC2_EVENT_MESSAGE with end stream in this
           * stream. This is the last.
           */


          /* You may also send a status "EZGRPC2_STATUS_DATA_LOSS", when the message
           * failed to be deserialized.
           */
          data = create_userdata(
              event->session_uuid, event->dataloss.stream_id,
              (list_t){NULL, NULL},
              1, EZGRPC2_STATUS_DATA_LOSS);

          if (path_userdata->is_unary)
            pthpool_add_task(upool, path_userdata->callback, data, NULL, NULL);
          else
            pthpool_add_task(opool, path_userdata->callback, data, NULL, NULL);

          break;
        case EZGRPC2_EVENT_CANCEL:
          printf("event cancel on stread %d\n\n", event->cancel.stream_id);
          // FIXME: HANDLE ME
          break;
      } /* switch */
      free(event);
    } /* while() */
  } /* for() */
}

static void handle_thread_pool(ezgrpc2_server_t *server, pthpool_t *pool) {
 list_t list_tasks;
 /* retrieve any finished tasks */
 pthpool_poll(pool, &list_tasks);
 task_t *task;
 while ((task = list_popb(&list_tasks)) != NULL) {
   struct userdata_t *data = task->ret;
   if (task->is_timeout) {
     assert(task->ret == NULL);
     free_list_messages(&data->list_imessages);
     free(task->userdata);
     /* when task has timeout, it is not executed so
      * task->ret is always NULL */
     free(task->ret); // No effect
     free(task);
     continue;
   }
   /* ezgrpc2_session_send() will take ownership of list of messages if it succeeds,
    * otherwise you will have to manually free it.
    */
   switch (ezgrpc2_session_send(server, data->session_uuid, data->stream_id, data->list_omessages)){
     case 0:
      /* ok */
       if (data->end_stream)
         ezgrpc2_session_end_stream(server, data->session_uuid, data->stream_id, data->status);
       break;
     case 1:
       /* possibly the client closed the connection */
       /* free */
       free_list_messages(&data->list_omessages);
       break;
     case 2:
       /* possibly the client sent a rst stream */
       printf("stream id doesn't exists %d\n", data->stream_id);
       /* free */
       free_list_messages(&data->list_omessages);
       break;
     default:
       assert(0);
   } /* switch */
   free(task->ret);
   free(task);
  } /* while() */
}





///////////////////////////////////////////////////////////





int main() {
  int res;
  const size_t nb_paths = 2;
  ezgrpc2_path_t paths[nb_paths];
  struct path_userdata_t path_userdata[nb_paths];
#ifdef __unix__
  /* In a real application, user must configure the server
   * to handle SIGTERM, and make sure to prevent these
   * signal from propagating through the main threads and
   * pool threads via pthread_sigmask(SIG_BLOCK, ...) for
   * the signal handler and pthread_sigmask(SIG_SETMASK, ...)
   * for the rest. This is skipped.
   */
  signal(SIGPIPE, SIG_IGN);
#endif




#ifdef HAVE_SECCOMP
#define BLACKLIST(ctx, name)                                                   \
  do {                                                                         \
    if ((res = seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(name), 0))) {     \
      fprintf(stderr, "seccomp error %d\n", res);                              \
      goto seccomp_fail;                                                       \
    }                                                                          \
  } while (0)
  /* Fancy security to prevent shell executions.
   * Maybe trap them and kill the server gracefully.*/
  scmp_filter_ctx seccomp_ctx = seccomp_init(SCMP_ACT_ALLOW);
  assert(seccomp_ctx != 0);
  BLACKLIST(seccomp_ctx, execve);
  BLACKLIST(seccomp_ctx, fork);
  //BLACKLIST(seccomp_ctx, seccomp);
  seccomp_load(seccomp_ctx);
#endif





  /* Tasks for unary requests (single message with end stream) */
  pthpool_t *unordered_pool = NULL;
  /* Tasks for streaming requests (multiple messages in a stream).
   * streaming rpc is generally slower than unary request since
   * the messages must be ordered and hence can't be parallelize */
  pthpool_t *ordered_pool = NULL;

  unordered_pool = pthpool_init(2, -1);
  assert(unordered_pool != NULL);

  /* worker must be one for an ordered execution */
  ordered_pool = pthpool_init(1, -1);
  assert(ordered_pool != NULL);
  //ezgrpc2_server_t *server = ezgrpc2_server_init("127.0.0.1", 19009, NULL, -1, 16);
  ezgrpc2_server_t *server = ezgrpc2_server_init("0.0.0.0", 19009, "::", 19009, 16);
  assert(server != NULL);


  /*-----------------------------.
  | What services do we provide? |
  `-----------------------------*/

  paths[0].path = "/test.yourAPI/whatever_service1";
  paths[1].path = "/test.yourAPI/whatever_service2";
  list_init(&paths[0].list_events);
  list_init(&paths[1].list_events);
  /* we expect to receive one or more grpc message in a
   * single stream.
   * */
  path_userdata[0].is_unary = 0;
  path_userdata[0].callback = callback_path0;
  paths[0].userdata = path_userdata + 0;
  /* we expect to receive only one grpc message in a
   * single stream
   */
  path_userdata[1].is_unary = 1;
  path_userdata[1].callback = callback_path1;
  paths[1].userdata = path_userdata + 1;





  //  gRPC allows clients to make concurent requests to a server in
  //  a single connection by making use of stream ids in HTTP2.
  //  
  //  A gRPC request to a server, at it's bare minimum, must have a `path`
  //  and one or more `grpc message`s.
  //  
  //  When a client makes a requests, that request is not necessarily immediately
  //  executed, instead, the request is added to the queue in the thread pool,
  //  waiting to be executed among other requests.
  //  
  //  When the tasks has been executed, the result is added to the finished
  //  queue, waiting to be pulled off with, `pthpool_poll()`. After that
  //  we can then send our results.
  //  
  //  So basically, we have a loop of:
  //  
  //    1. Get server events. (server poll)
  //    2. Add tasks to the thread pool. (give the task to thread pool).
  //    3. Get finish tasks from the thread pool (thread pool poll)
  //    4. Send the results. (give the result to the client)


  while (1) {
    int is_pool_empty = pthpool_is_empty(unordered_pool) &&
                        pthpool_is_empty(ordered_pool);

    /* if thread pool is empty, maybe we can give our resources to the cpu
     * and wait a little longer.
     */
    int timeout = is_pool_empty ? 10000 : 10;

#ifdef __unix__
    // if sigterm flag has been set by the signal handler, break the loop and kill
    // server.

    /*   .... */
#endif
    // step 1. server poll
    if ((res = ezgrpc2_server_poll(server, paths, nb_paths, timeout)) < 0)
      break;

    if (res > 0) {
      // step 2. Give the task to the thread pool
      handle_events(server, paths, nb_paths, ordered_pool, unordered_pool);
    }
    else if (res == 0) {
      printf("no event\n");
      // No server events, let's check the thread pool.
    }
    // step 3, 4 Get finish tasks and send results
    handle_thread_pool(server, ordered_pool);
    handle_thread_pool(server, unordered_pool);
  }

  if (res < 0) {
    printf("poll err\n");
  }


  ezgrpc2_server_destroy(server);
  pthpool_destroy(ordered_pool);
  pthpool_destroy(unordered_pool);

#ifdef HAVE_SECCOMP
  seccomp_release(seccomp_ctx);
seccomp_fail:
#endif
  return res;
}
