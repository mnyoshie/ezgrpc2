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

#ifdef __unix__
#include <signal.h>
#include <pthread.h>
#endif

#include "ezgrpc2.h"

struct path_userdata_t {
  char is_unary;
  ezgrpc2_list_t *(*callback)(ezgrpc2_list_t *);
};


void free_lmessages(ezgrpc2_list_t *lmessages){
  ezgrpc2_message_t *msg;
  while ((msg = ezgrpc2_list_pop_front(lmessages)) != NULL) {
    ezgrpc2_message_free(msg);
  }
  ezgrpc2_list_free(lmessages);
}

void free_levents(ezgrpc2_list_t *levents) {
  ezgrpc2_event_t *event;
  while ((event = ezgrpc2_list_pop_front(levents)) != NULL) {
    ezgrpc2_event_free(event);
  }
  ezgrpc2_list_free(levents);
}

_Atomic int pp = 0;
ezgrpc2_list_t *callback_path0(ezgrpc2_list_t *lmessages){
  ezgrpc2_list_t *lmessages_ret = ezgrpc2_list_new(NULL);
  /* if client did not request properly, skipp processing. */
  /* pretend this is our response (output messages) */
  for (int i = 0; i < 2; i++, pp++) {
    ezgrpc2_message_t *msg = ezgrpc2_message_new(11);
    msg->is_compressed = 0;
    memcpy(msg->data, "    path1!", 11);
    *(uint32_t*)(msg->data) = pp;
    
    ezgrpc2_list_push_back(lmessages_ret, msg);
  }

  /* cleanup (input messages) */
  return lmessages_ret;
}
ezgrpc2_list_t *callback_path1(ezgrpc2_list_t *lmessages){
  ezgrpc2_list_t *lmessages_ret = ezgrpc2_list_new(NULL);
  for (int i = 0; i < 2; i++, pp++) {
    ezgrpc2_message_t *msg = ezgrpc2_message_new(11);
    msg->is_compressed = 0;
    memcpy(msg->data, "    path1!", 11);
    *(uint32_t*)(msg->data) = pp;
    ezgrpc2_list_push_back(lmessages_ret, msg);
  }

  /* cleanup (input messages) */
  return lmessages_ret;
}




static void handle_event_message(ezgrpc2_event_t *event,
                                 struct path_userdata_t *path_userdata,
                                 ezgrpc2_server_t *server) {
  size_t nb_msg = ezgrpc2_list_count(event->message.lmessages);
  if (path_userdata->is_unary) {
    if (event->message.end_stream == 0 || nb_msg != 1) {
      /* This is unary service, but they are sending more than one message */
      ezgrpc2_session_end_stream(server, event->session_uuid,
                                 event->message.stream_id,
                                 EZGRPC2_STATUS_INVALID_ARGUMENT);
      return;
    }
  }
  ezgrpc2_list_t *lmessages = path_userdata->callback(event->message.lmessages);
  switch (ezgrpc2_session_send(server, event->session_uuid, event->message.stream_id, lmessages)){
    case 0:
     /* ok */
      if (event->message.end_stream)
        ezgrpc2_session_end_stream(server, event->session_uuid, event->message.stream_id, EZGRPC2_STATUS_OK);
      break;
    case 1:
      break;
    case 2:
      printf(">stream id doesn't exists %d\n", event->message.stream_id);
      break;
    default:
      assert(0);
  } /* switch */
  free_lmessages(lmessages);

}

static void handle_event_dataloss(ezgrpc2_event_t *event,
                                 struct path_userdata_t *path_userdata,
                                 ezgrpc2_server_t *server) {
   ezgrpc2_session_end_stream(server, event->session_uuid, event->dataloss.stream_id, EZGRPC2_STATUS_DATA_LOSS);
}

static void handle_events(ezgrpc2_server_t *server, ezgrpc2_path_t *paths,
                          size_t nb_paths) {
  ezgrpc2_event_t *event;
  struct userdata_t *data = NULL;
  struct path_userdata_t *path_userdata = NULL;
  /* check for events in the paths */
  for (size_t i = 0; i < nb_paths; i++) {
    path_userdata = paths[i].userdata;
    while ((event = ezgrpc2_list_pop_front(paths[i].levents)) != NULL) {
      switch (event->type) {
      case EZGRPC2_EVENT_MESSAGE:
        handle_event_message(event, path_userdata, server);
        break;
      case EZGRPC2_EVENT_DATALOSS:
        handle_event_dataloss(event, path_userdata, server);
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
    if (sigterm_flag) {
      break;
    }

#endif
    /* if thread pool is empty, maybe we can give our resources to the cpu
     * and wait a little longer.
     */
    int timeout = 10000;

    // step 1. server poll
    if ((res = ezgrpc2_server_poll(server, paths, nb_paths, timeout)) < 0)
      break;

    if (res > 0) {
      puts("incoming events");
      // step 2. Give the task to the thread pool
      handle_events(server, paths, nb_paths);
    }
    else if (res == 0) {
      printf("no event\n");
      // No server events, let's check the thread pool.
    }
  }

  if (res < 0) {
    printf("poll err\n");
  }


  ezgrpc2_server_free(server);
  /* we aee aure these are enpty because we did not poll at break */
  ezgrpc2_list_free(paths[0].levents);
  ezgrpc2_list_free(paths[1].levents);

  return res;
}
