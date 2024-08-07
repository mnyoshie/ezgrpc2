/* The author disclaims copyright to this source code
 * and releases it into the public domain.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "ezgrpc2.h"
#include "pthpool.h"

struct userdata_t {
  i32 stream_id;
  uint8_t session_id[32];
  char end_stream;
  int status;
  list_t list_imessages, list_omessages;
};

pthpool_t *pool = NULL;

struct userdata_t *create_userdata(uint8_t session_id[32], int32_t stream_id,
    list_t list_messages, char end_stream, int status) {
  struct userdata_t *data = malloc(sizeof(*data));
  memcpy(data->session_id, session_id, 32);
  data->stream_id = stream_id;
  data->list_imessages = list_messages;
  data->end_stream = end_stream;
  data->status = status;
  return data;
}

int pp = 0;
void *callback(void *data){
  printf("task executed!!\n");
  struct userdata_t *usrdata = data;
  list_init(&usrdata->list_omessages);
  if (usrdata->status)
    return usrdata;

  /* pretend this is our response (output messages) */
  for (int i = 0; i < 5; i++, pp++) {
    ezgrpc2_message_t *msg = malloc(sizeof(*msg));
    msg->is_compressed = 0;
    msg->data = strdup("    Hello world!!");
    
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

static void handle_events(ezgrpc2_server_t *server, ezgrpc2_path_t *paths, size_t nb_paths) {
  ezgrpc2_event_t *event;
  struct userdata_t *data = NULL;
  /* check for events in the paths */
  for (size_t i = 0; i < nb_paths; i++) {
    while ((event = list_popb(&paths[i].list_events)) != NULL) {
      switch(event->type) {
        case EZGRPC2_EVENT_MESSAGE:
          printf("event message %zu end stream %d\n", list_count(&event->message.list_messages), event->message.end_stream);
#if 1
          data = create_userdata(
              event->session_id, event->message.stream_id,
              event->message.list_messages,
              event->message.end_stream, 0);
          /* we've taken ownership of this. clear */
          list_init(&event->message.list_messages);

          /* add task to pool */
          pthpool_add_task(pool, callback, data, NULL, NULL);
#else
          ezgrpc2_session_end_session(server, event->session_id, 0, 0);
#endif
          break;
        case EZGRPC2_EVENT_DATALOSS:
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
           * stream. This the last.
           * */
          data = create_userdata(
              event->session_id, event->dataloss.stream_id,
              (list_t){NULL, NULL},
              1, EZGRPC2_STATUS_DATA_LOSS);
          pthpool_add_task(pool, callback, data, NULL, NULL);
          printf("event dataloss\n");
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

static void handle_thread_pool(ezgrpc2_server_t *server) {
 list_t list_tasks;
 /* retrieve any finished tasks */
 pthpool_poll(pool, &list_tasks);
 task_t *task;
 while ((task = list_popb(&list_tasks)) != NULL) {
   struct userdata_t *data = task->ret;
   if (task->is_timeout) {
     /* when task has timeout, it is not executed so
      * task->ret is always NULL */
     assert(task->ret == NULL);
     /* make sure to also cleanup the userdata struct which
      */
     ezgrpc2_message_t *msg;
     /* free */
     while ((msg = list_popb(&data->list_imessages)) != NULL) {
       free(msg->data);
       free(msg);
     }
     free(task->userdata);
     free(task);
     continue;
   }
   /* ezgrpc2_session_send() will take ownership of list of messages if it succeeds,
    * otherwise you will have to manually free it.
    */
   switch (ezgrpc2_session_send(server, data->session_id, data->stream_id, &data->list_omessages)){
     case 0:
      /* ok */
       if (data->end_stream)
         ezgrpc2_session_end_stream(server, data->session_id, data->stream_id, data->status);
       break;
     case 1:
       /* possibly the client closed the connection */
       printf("session id doesn't exists\n");
       ezgrpc2_message_t *msg;
       /* free */
       while ((msg = list_popb(&data->list_omessages)) != NULL) {
         free(msg->data);
         free(msg);
       }
       break;
     case 2:
       /* possibly the client sent a rst stream */
       printf("stream id doesn't exists\n");
       /* free */
       while ((msg = list_popb(&data->list_omessages)) != NULL) {
         free(msg->data);
         free(msg);
       }
       break;
     default:
       assert(0);
   } /* switch */
   free(task->ret);
   free(task);
  } /* while() */
}

int main() {
#ifdef __unix__
  signal(SIGPIPE, SIG_IGN);
#endif

  pool = pthpool_init(1, -1);
  assert(pool != NULL);
  //ezgrpc2_server_t *server = ezgrpc2_server_init("127.0.0.1", 19009, NULL, -1, 16);
  ezgrpc2_server_t *server = ezgrpc2_server_init(NULL, -1, "::", 19009, 16);
  assert(server != NULL);

  const size_t nb_paths = 2;
  ezgrpc2_path_t paths[nb_paths];
  paths[0].path = "/test.yourAPI/whatever_service1";
  paths[1].path = "/test.yourAPI/someservice";
  list_init(&paths[0].list_events);
  list_init(&paths[1].list_events);




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
  //  queue, waiting to be pulled off with, `thpool_poll()`. After that
  //  we can then send our results.
  //  
  //  So basically, we have a loop of:
  //  
  //    1. Get server events. (server poll)
  //    2. Add tasks to the thread pool. (give the task to thread pool).
  //    3. Get finish tasks from the thread pool (thread pool poll)
  //    4. Send the results. (give the result to the client)


  int res;
  while (1) {
    /* if thread pool is empty, maybe we can give our resources to the cpu
     * and wait a little longer.
     */
    res = ezgrpc2_server_poll(server, paths, nb_paths, pthpool_is_empty(pool) ? 10000 : 5);
    if (res > 0) {
      handle_events(server, paths, nb_paths);
    } /* if (res > 0) */
    else if (res < 0) {
      printf("poll error\n");
      goto exit;
    }
    else if (res == 0) {
      printf("no event\n");
    }
    handle_thread_pool(server);
  }


exit:
  ezgrpc2_server_destroy(server);
  pthpool_destroy(pool);
  return res;
}
