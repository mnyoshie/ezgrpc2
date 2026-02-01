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

#include "ezgrpc2.h"
#include "ezgrpc2_pthpool.h"

#ifdef __unix__
#include <signal.h>
#include <pthread.h>
#endif

struct userdata_t {
  i32 stream_id;
  ezgrpc2_session_uuid *session_uuid;
  char end_stream;
  ezgrpc2_grpc_status status;

  /* input messages. requested by the client */
  ezgrpc2_list *limessages;

  /* output messages. our server response */
  ezgrpc2_list *lomessages;
};

struct path_userdata_t {
  char is_unary;
  void *(*callback)(void*);
};

/* pops all the messages and frees the list */
void free_lmessages(ezgrpc2_list *lmessages){
  if (lmessages == NULL)
    return;
  ezgrpc2_message *msg;
  while ((msg = ezgrpc2_list_pop_front(lmessages)) != NULL) {
    ezgrpc2_message_free(msg);
  }
  ezgrpc2_list_free(lmessages);
}


struct userdata_t *create_userdata(ezgrpc2_session_uuid *session_uuid, int32_t stream_id,
    ezgrpc2_list *lmessages, char end_stream, ezgrpc2_grpc_status status) {
  struct userdata_t *data = malloc(sizeof(*data));
  data->session_uuid = session_uuid;
  data->stream_id = stream_id;
  data->limessages = lmessages;
  data->lomessages = NULL;
  data->end_stream = end_stream;
  data->status = status;
  return data;
}

void free_userdata(struct userdata_t *userdata) {
  ezgrpc2_session_uuid_free(userdata->session_uuid);
  free_lmessages(userdata->limessages);
  free_lmessages(userdata->lomessages);
  free(userdata);
}


_Atomic int pp = 0;
/* No ezgrpc2_* functions must be called in this callback */
void *callback_path0(void *data){
  printf("task executed path0!!\n");
  struct userdata_t *userdata = data;
  userdata->lomessages = ezgrpc2_list_new(NULL);
  /* if client did not request properly, skipp processing. */
  if (userdata->status != EZGRPC2_GRPC_STATUS_OK)
    return userdata;

  /* pretend this is our response (output messages) */
  for (int i = 0; i < 5; i++, pp++) {
    ezgrpc2_message *msg = ezgrpc2_message_new(0, "    path0!", 11);
    *(uint32_t*)(msg->data) = pp;
    ezgrpc2_list_push_back(userdata->lomessages, msg);
  }

  return data;
}

/* No ezgrpc2_* functions must be called in this callback */
void *callback_path1(void *data){
  printf("task executed path1!!\n");
  struct userdata_t *userdata = data;
  userdata->lomessages = ezgrpc2_list_new(NULL);
  /* if client did not request properly, skipp processing. */
  if (userdata->status != EZGRPC2_GRPC_STATUS_OK)
    return userdata;

  /* pretend this is our response (output messages) */
  for (int i = 0; i < 5; i++, pp++) {
    ezgrpc2_message *msg = ezgrpc2_message_new(0, "    path1!", 11);
    *(uint32_t*)(msg->data) = pp;
    ezgrpc2_list_push_back(userdata->lomessages, msg);
  }


  /* cleanup (input messages) */
  free_lmessages(userdata->limessages);
  userdata->limessages = NULL;
  return data;
}


static void handle_event_message(ezgrpc2_event *event,
                                 ezgrpc2_server *server,
                                 ezgrpc2_pthpool *opool,
                                 ezgrpc2_pthpool *upool) {
  size_t nb_msg = ezgrpc2_list_count(event->message.lmessages);
  printf("stream_id %d, event message. nb_message %zu. end stream %d\n", event->message.stream_id,
         nb_msg, event->message.end_stream);

  struct path_userdata_t *path_userdata = event->userdata;
  if (path_userdata->is_unary) {
    if (event->message.end_stream == 0 || nb_msg != 1) {
      /* This is unary service, but they are sending more than one message */
      ezgrpc2_server_session_stream_end(server, event->session_uuid,
                                 event->message.stream_id,
                                 EZGRPC2_GRPC_STATUS_INVALID_ARGUMENT);
      return;
    }
  }
#if 1
  struct userdata_t *data = create_userdata(
      ezgrpc2_session_uuid_copy(event->session_uuid), event->message.stream_id, event->message.lmessages,
      event->message.end_stream, 0 /* status ok */);
  /* we've taken ownership of this addresses. set to NULL to prevent ezgrpc2_event_free
   * from freeing it. */
   event->message.lmessages = NULL;

  /* if path/service is unary, add task to upool, else add to opool */
  if (path_userdata->is_unary)
    ezgrpc2_pthpool_add_task(upool, path_userdata->callback, data, NULL, NULL);
  else
    ezgrpc2_pthpool_add_task(opool, path_userdata->callback, data, NULL, NULL);
#else
  ezgrpc2_session_end_session(server, event->session_uuid, 0, 0);
#endif
}

static void handle_event_dataloss(ezgrpc2_event *event,
                                 ezgrpc2_server *server,
                                 ezgrpc2_pthpool *opool,
                                 ezgrpc2_pthpool *upool) {
  printf("stream_id %d, event dataloss\n", event->dataloss.stream_id);
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
   * We will not be getting a EZGRPC2_EVENT_MESSAGE with end stream in
   * this stream. This is the last.
   */

  /* You may also send a status "EZGRPC2_GRPC_STATUS_DATA_LOSS", when the
   * message failed to be deserialized.
   */
                
  struct path_userdata_t *path_userdata = event->userdata;
  struct userdata_t *data = create_userdata(ezgrpc2_session_uuid_copy(event->session_uuid), event->dataloss.stream_id,
                         ezgrpc2_list_new(NULL), 1,
                         EZGRPC2_GRPC_STATUS_DATA_LOSS);
  /* weve taken ownershio of this address. clear */


  if (path_userdata->is_unary)
    ezgrpc2_pthpool_add_task(upool, path_userdata->callback, data, NULL,
                             NULL);
  else
    ezgrpc2_pthpool_add_task(opool, path_userdata->callback, data, NULL,
                             NULL);

}

static void handle_events(ezgrpc2_server *server, ezgrpc2_list *levents, ezgrpc2_path *paths,
                          ezgrpc2_pthpool *opool,
                          ezgrpc2_pthpool *upool) {
  ezgrpc2_event *event;
  /* check for events in the paths */
  while ((event = ezgrpc2_list_pop_front(levents)) != NULL) {
    switch (event->type) {
    case EZGRPC2_EVENT_MESSAGE:
      handle_event_message(event, server, opool, upool);
      break;
    case EZGRPC2_EVENT_DATALOSS:
      handle_event_dataloss(event, server, opool, upool);
      break;
    case EZGRPC2_EVENT_CANCEL:
      printf("event cancel on stread %d\n\n", event->cancel.stream_id);
      // FIXME: HANDLE ME
      break;
    case EZGRPC2_EVENT_CONNECT:
      printf("event connect\n");
      break;
    case EZGRPC2_EVENT_DISCONNECT:
      printf("event disconnect\n");
      break;
    
    } /* switch */
    ezgrpc2_event_free(event);
  } /* while() */
}

static void handle_thread_pool(ezgrpc2_server *server, ezgrpc2_pthpool *pool) {
 ezgrpc2_list *lresults = ezgrpc2_list_new(NULL);
 /* retrieve any finished tasks */
 ezgrpc2_pthpool_poll(pool, lresults);
 ezgrpc2_pthpool_result_t *result;
 while ((result = ezgrpc2_list_pop_front(lresults)) != NULL) {
   struct userdata_t *data = result->ret;
   if (result->is_timeout) {
     free_userdata(result->userdata);
     /* when task has timeout, it is not executed so
      * result->ret is always NULL */
     ezgrpc2_pthpool_result_free(result);
     continue;
   }
   /* ezgrpc2_session_send() will empty data->lomessages if it succeeds.
    */
   switch (ezgrpc2_server_session_stream_send(server, data->session_uuid, data->stream_id, data->lomessages)){
     case 0:
      /* ok */
       if (data->end_stream)
         ezgrpc2_server_session_stream_end(server, data->session_uuid, data->stream_id, data->status);
       break;
     case 1:
       break;
     case 2:
       /* possibly the client sent a rst stream */
       printf("stream id doesn't exists %d\n", data->stream_id);
       /* free */
       break;
     case 3:
       printf("fatal\n");
       break;
     default:
       assert(0);
   } /* switch */
   ezgrpc2_pthpool_result_free(result);
   free_userdata(data);
  } /* while() */
  ezgrpc2_list_free(lresults);
}





///////////////////////////////////////////////////////////




#ifdef __unix__
_Atomic int sigterm_flag = 0;
static void *signal_handler(void *data) {
  sigset_t *sigset = data;
  int sig, res;
  while (1) {
    res = sigwait(sigset, &sig);
    if (res > 1) {
      fprintf(stderr, "sigwait error\n");
      abort();
    }
    if (sig & SIGTERM) {
      sigterm_flag = 1;
      write(0, "sigterm_flag set. waiting for next poll timeout\n", 48);
    }
  }
}
#endif

int main() {
  int res;
  const size_t nb_paths = 2;
  ezgrpc2_path paths[nb_paths];
  struct path_userdata_t path_userdata[nb_paths];
  res = ezgrpc2_global_init(0);
  if (res) 
    abort();


#ifdef __unix__
  /* In a real application, user must configure the server
   * to handle SIGTERM, and make sure to prevent these
   * signal from propagating through the main threads and
   * pool threads via pthread_sigmask(SIG_BLOCK, ...)
   */
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGTERM);
  sigaddset(&sigset, SIGPIPE);
  res = pthread_sigmask (SIG_BLOCK, &sigset, NULL);
  if (res != 0) {
    fprintf(stderr, "pthread_sigmask failed\n");
    abort();
  }

  /* run our signal handler */
  pthread_t sig_thread;
  res = pthread_create(&sig_thread, NULL, signal_handler, &sigset);
  if (res) {
    fprintf(stderr, "pthread_create failed\n");
    abort();
  }
#endif /* __unix__ */

  /* Tasks for unary requests (single message with end stream) */
  ezgrpc2_pthpool *unordered_pool = NULL;
  /* Tasks for streaming requests (multiple messages in a stream).
   * streaming rpc is generally slower than unary request since
   * the messages must be ordered and hence can't be parallelize */
  ezgrpc2_pthpool *ordered_pool = NULL;

  unordered_pool = ezgrpc2_pthpool_new(2, 0);
  assert(unordered_pool != NULL);

  /* worker must be one for an ordered execution */
  ordered_pool = ezgrpc2_pthpool_new(1, 0);
  assert(ordered_pool != NULL);

  /* setup server settings */
  ezgrpc2_server_settings *server_settings = ezgrpc2_server_settings_new(NULL);
  assert(server_settings != NULL);
  ezgrpc2_server_settings_set_log_level(server_settings, EZGRPC2_SERVER_LOG_ALL);
  ezgrpc2_server_settings_set_log_fp(server_settings, stderr);

  /* The heart of this API */
  ezgrpc2_server *server = ezgrpc2_server_new("0.0.0.0", 19009, "::", 19009, 16, server_settings, NULL);
  ezgrpc2_server_settings_free(server_settings);
  assert(server != NULL);


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


  /*-----------------------------.
  | What services do we provide? |
  `-----------------------------*/

  (void)ezgrpc2_server_register_path(server, "/test.yourAPI/whatever_service1", path_userdata, 0, 0);
  (void)ezgrpc2_server_register_path(server, "/test.yourAPI/whatever_service2", path_userdata + 1, 0, 0);




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
  //  queue, waiting to be pulled off with, `ezgrpc2_pthpool_poll()`. After that
  //  we can then send our results.
  //  
  //  So basically, we have a loop of:
  //  
  //    1. Get server events. (server poll)
  //    2. Add tasks to the thread pool. (give the task to thread pool).
  //    3. Get finish tasks from the thread pool (thread pool poll)
  //    4. Send the results. (give the result to the client)


  ezgrpc2_list *levents = ezgrpc2_list_new(NULL);
  while (1) {
#ifdef __unix__
    // if sigterm flag has been set by the signal handler, break the loop and kill
    // server.
    if (sigterm_flag)
      break;

#endif
    int is_pool_empty = ezgrpc2_pthpool_is_empty(unordered_pool) &&
                        ezgrpc2_pthpool_is_empty(ordered_pool);

    /* if thread pool is empty, maybe we can give our resources to the cpu
     * and wait a little longer.
     */
    int timeout = is_pool_empty ? 10000 : 10;
    // step 1. server poll
    if ((res = ezgrpc2_server_poll(server, levents, timeout)) < 0)
      break;

    if (res > 0) {
      puts("incoming events");
      // step 2. Give the task to the thread pool
      handle_events(server, levents, paths, ordered_pool, unordered_pool);
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

  puts("exiting");
  /* we are sure these are empty because we break the while loop before polling */
  ezgrpc2_list_free(levents);
  ezgrpc2_server_free(server);
  ezgrpc2_pthpool_free(ordered_pool);
  ezgrpc2_pthpool_free(unordered_pool);

  ezgrpc2_global_cleanup();
  return res;
}
