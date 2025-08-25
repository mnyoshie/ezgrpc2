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
#include "ezgrpc2_pthpool.h"

#ifdef __unix__
#include <pthread.h>
#endif

struct userdata_t {
  i32 stream_id;
  ezgrpc2_session_uuid_t *session_uuid;
  char end_stream;
  ezgrpc2_grpc_status_t status;

  /* input messages. requested by the client */
  ezgrpc2_list_t *limessages;

  /* output messages. our server response */
  ezgrpc2_list_t *lomessages;
};

struct path_userdata_t {
  char is_unary;
  void *(*callback)(void*);
};


struct userdata_t *create_userdata(ezgrpc2_session_uuid_t *session_uuid, int32_t stream_id,
    ezgrpc2_list_t *lmessages, char end_stream, ezgrpc2_grpc_status_t status) {
  struct userdata_t *data = malloc(sizeof(*data));
  data->session_uuid = ezgrpc2_session_uuid_copy(session_uuid);
  data->stream_id = stream_id;
  data->limessages = lmessages;
  data->end_stream = end_stream;
  data->status = status;
  return data;
}

void free_lmessages(ezgrpc2_list_t *lmessages){
  ezgrpc2_message_t *msg;
  while ((msg = ezgrpc2_list_pop_front(lmessages)) != NULL) {
    free(msg->data);
    free(msg);
  }
  ezgrpc2_list_free(lmessages);
}

_Atomic int pp = 0;
/* No ezgrpc2_* functions must be called in this callback */
void *callback_path0(void *data){
  printf("task executed path0!!\n");
  struct userdata_t *userdata = data;
  userdata->lomessages = ezgrpc2_list_new(NULL);
  /* if client did not request properly, skipp processing. */
  if (userdata->status != EZGRPC2_STATUS_OK)
    return userdata;

  /* pretend this is our response (output messages) */
  for (int i = 0; i < 5; i++, pp++) {
    ezgrpc2_message_t *msg = malloc(sizeof(*msg));
    msg->is_compressed = 0;
    msg->data = (void*)strdup("    path0!");
    
    msg->len = strlen((char*)msg->data) + 1;
    *(uint32_t*)(msg->data) = pp;
    ezgrpc2_list_push_back(userdata->lomessages, msg);
  }

  /* cleanup (input messages) */
  free_lmessages(userdata->limessages);
  userdata->limessages = NULL;

  return data;
}

/* No ezgrpc2_* functions must be called in this callback */
void *callback_path1(void *data){
  printf("task executed path1!!\n");
  struct userdata_t *userdata = data;
  userdata->lomessages = ezgrpc2_list_new(NULL);
  /* if client did not request properly, skipp processing. */
  if (userdata->status != EZGRPC2_STATUS_OK)
    return userdata;

  /* pretend this is our response (output messages) */
  for (int i = 0; i < 5; i++, pp++) {
    ezgrpc2_message_t *msg = malloc(sizeof(*msg));
    msg->is_compressed = 0;
    msg->data = (void*)strdup("    path1!");
    
    msg->len = strlen((char*)msg->data) + 1;
    *(uint32_t*)(msg->data) = pp;
    ezgrpc2_list_push_back(userdata->lomessages, msg);
  }


  /* cleanup (input messages) */
  free_lmessages(userdata->limessages);
  userdata->limessages = NULL;
  return data;
}


static void handle_event_message(ezgrpc2_event_t *event,
                                 struct path_userdata_t *path_userdata,
                                 ezgrpc2_server_t *server,
                                 ezgrpc2_pthpool_t *opool,
                                 ezgrpc2_pthpool_t *upool) {
  size_t nb_msg = ezgrpc2_list_count(event->message.lmessages);
  printf("stream_id %d, event message. nb_message %zu. end stream %d\n", event->message.stream_id,
         nb_msg, event->message.end_stream);

  if (path_userdata->is_unary) {
    if (event->message.end_stream == 0 || nb_msg != 1) {
      /* This is unary service, but they are sending more than one message */
      ezgrpc2_session_end_stream(server, event->session_uuid,
                                 event->message.stream_id,
                                 EZGRPC2_STATUS_INVALID_ARGUMENT);
      free_lmessages(event->message.lmessages);
      return;
    }
  }
#if 1
  struct userdata_t *data = create_userdata(
      event->session_uuid, event->message.stream_id, event->message.lmessages,
      event->message.end_stream, 0 /* status ok */);
  /* we've taken ownership of this. clear */
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

static void handle_event_dataloss(ezgrpc2_event_t *event,
                                 struct path_userdata_t *path_userdata,
                                 ezgrpc2_server_t *server,
                                 ezgrpc2_pthpool_t *opool,
                                 ezgrpc2_pthpool_t *upool) {
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

  /* You may also send a status "EZGRPC2_STATUS_DATA_LOSS", when the
   * message failed to be deserialized.
   */
  struct userdata_t *data = create_userdata(event->session_uuid, event->dataloss.stream_id,
                         ezgrpc2_list_new(NULL), 1,
                         EZGRPC2_STATUS_DATA_LOSS);

  if (path_userdata->is_unary)
    ezgrpc2_pthpool_add_task(upool, path_userdata->callback, data, NULL,
                             NULL);
  else
    ezgrpc2_pthpool_add_task(opool, path_userdata->callback, data, NULL,
                             NULL);

}

static void handle_events(ezgrpc2_server_t *server, ezgrpc2_path_t *paths,
                          size_t nb_paths, ezgrpc2_pthpool_t *opool,
                          ezgrpc2_pthpool_t *upool) {
  ezgrpc2_event_t *event;
  struct userdata_t *data = NULL;
  struct path_userdata_t *path_userdata = NULL;
  /* check for events in the paths */
  for (size_t i = 0; i < nb_paths; i++) {
    path_userdata = paths[i].userdata;
    while ((event = ezgrpc2_list_pop_front(paths[i].levents)) != NULL) {
      switch (event->type) {
      case EZGRPC2_EVENT_MESSAGE:
        handle_event_message(event, path_userdata, server, opool, upool);
        break;
      case EZGRPC2_EVENT_DATALOSS:
        handle_event_dataloss(event, path_userdata, server, opool, upool);
        break;
      case EZGRPC2_EVENT_CANCEL:
        printf("event cancel on stread %d\n\n", event->cancel.stream_id);
        // FIXME: HANDLE ME
        break;
      } /* switch */
      ezgrpc2_event_free(event);
    } /* while() */
  } /* for() */
}

static void handle_thread_pool(ezgrpc2_server_t *server, ezgrpc2_pthpool_t *pool) {
 ezgrpc2_list_t *lresults = ezgrpc2_list_new(NULL);
 /* retrieve any finished tasks */
 ezgrpc2_pthpool_poll(pool, lresults);
 ezgrpc2_pthpool_result_t *result;
 while ((result = ezgrpc2_list_pop_front(lresults)) != NULL) {
   struct userdata_t *data = result->ret;
   if (result->is_timeout) {
     free_lmessages(data->limessages);
     free(result->userdata);
     /* when task has timeout, it is not executed so
      * task->ret is always NULL */
     ezgrpc2_pthpool_result_free(result);
     continue;
   }
   /* ezgrpc2_session_send() will take ownership of list of messages if it succeeds,
    * otherwise you will have to manually free it.
    */
   switch (ezgrpc2_session_send(server, data->session_uuid, data->stream_id, data->lomessages)){
     case 0:
      /* ok */
       free_lmessages(data->lomessages);
       if (data->end_stream)
         ezgrpc2_session_end_stream(server, data->session_uuid, data->stream_id, data->status);
       break;
     case 1:
       /* possibly the client closed the connection */
       /* free */
       free_lmessages(data->lomessages);
       break;
     case 2:
       /* possibly the client sent a rst stream */
       printf("stream id doesn't exists %d\n", data->stream_id);
       /* free */
       free_lmessages(data->lomessages);
       break;
     default:
       assert(0);
   } /* switch */
   ezgrpc2_pthpool_result_free(result);
   ezgrpc2_session_uuid_free(data->session_uuid);
   free(data);
  } /* while() */
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
  ezgrpc2_path_t paths[nb_paths];
  struct path_userdata_t path_userdata[nb_paths];


#ifdef __unix__
  /* In a real application, user must configure the server
   * to handle SIGTERM, and make sure to prevent these
   * signal from propagating through the main threads and
   * pool threads via pthread_sigmask(SIG_BLOCK, ...) for
   * the signal handler and pthread_sigmask(SIG_SETMASK, ...)
   * for the rest. 
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

  /* make sure the signals won't propagate through the
   * main thread and worker threads
   */
  res = pthread_sigmask(SIG_SETMASK, &sigset, NULL);
  if (res != 0) {
    fprintf(stderr, "pthread_sigmask failed\n");
    abort();
  }
#endif /* __unix__ */

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
#undef BLACKLIST
#endif

  /* Tasks for unary requests (single message with end stream) */
  ezgrpc2_pthpool_t *unordered_pool = NULL;
  /* Tasks for streaming requests (multiple messages in a stream).
   * streaming rpc is generally slower than unary request since
   * the messages must be ordered and hence can't be parallelize */
  ezgrpc2_pthpool_t *ordered_pool = NULL;

  unordered_pool = ezgrpc2_pthpool_new(2, 0);
  assert(unordered_pool != NULL);

  /* worker must be one for an ordered execution */
  ordered_pool = ezgrpc2_pthpool_new(1, 0);
  assert(ordered_pool != NULL);

  /* The heart of this API */
  ezgrpc2_server_t *server = ezgrpc2_server_new("0.0.0.0", 19009, "::", 19009, 16, NULL);
  assert(server != NULL);


  /*-----------------------------.
  | What services do we provide? |
  `-----------------------------*/

  paths[0].path = "/test.yourAPI/whatever_service1";
  paths[1].path = "/test.yourAPI/whatever_service2";

  paths[0].levents = ezgrpc2_list_new(NULL);
  paths[1].levents = ezgrpc2_list_new(NULL);
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
  //  queue, waiting to be pulled off with, `ezgrpc2_pthpool_poll()`. After that
  //  we can then send our results.
  //  
  //  So basically, we have a loop of:
  //  
  //    1. Get server events. (server poll)
  //    2. Add tasks to the thread pool. (give the task to thread pool).
  //    3. Get finish tasks from the thread pool (thread pool poll)
  //    4. Send the results. (give the result to the client)


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
    if ((res = ezgrpc2_server_poll(server, paths, nb_paths, timeout)) < 0)
      break;

    if (res > 0) {
      puts("incoming events");
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


  ezgrpc2_server_free(server);
  /* we aee aure these are enpty because we did not poll at break */
  ezgrpc2_list_free(paths[0].levents);
  ezgrpc2_list_free(paths[1].levents);
  ezgrpc2_pthpool_free(ordered_pool);
  ezgrpc2_pthpool_free(unordered_pool);

#ifdef HAVE_SECCOMP
  seccomp_release(seccomp_ctx);
seccomp_fail:
#endif
  return res;
}
