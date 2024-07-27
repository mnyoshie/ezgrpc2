/* The author disclaims copyright to this source code and releases
 * it into the public domain;
 */
#include <stdio.h>
#include "ezgrpc2.h"


int volatile is_running = 1;
enum ezgrpc_event_type_t {
  EZGRPC2_EVENT_STREAM_MESSAGE,
  EZGRPC2_EVENT_STREAM_CANCEL,
};

struct ezgrpc2_message_t {
  char is_compressed;
  uint32_t len;
  char *data;
};

typedef struct ezgrpc2_event_message_t ezgrpc2_event_message_t;
struct ezgrpc2_event_message_t {
  void *stream_ctx;
  void *userdata;
  size_t nb_messages;
  ezgrpc2_message *messages;
};

typedef struct ezgrpc2_event_cancel_t ezgrpc2_event_cancel_t;
struct ezgrpc2_event_cancel_t {
  void *stream_ctx;
};

struct ezgrpc2_event_t {
  char *ip;
  ezgrpc_event_type_t event;
  union {
    ezgrpc2_event_message_t message;
    ezgrpc2_event_cancel_t cancel;
  };
};

struct userdata_t {
  void *(*callback)(void *);
};

void *callback(void *userdata) {
  void *r = strdup("Hello from callback!\n");
  return r;
}
int main(){

  ezgrpc2_server_t *server = ezgrpc2_server_init("127.0.0.1", 8080, NULL, -1, 0);
  thpool_t *pool = ezgrpc2_workers_init(4);

  ezgrpc2_server_add_path(server, "/whatever.PublicAPI/DoSomething1", 0, NULL); 
  ezgrpc2_server_add_path(server, "/whatever.PublicAPI/DoSomething2", 1, NULL); 

  assert(server != NULL);
  ezgrpc2_event_t *events;

  while (is_running) {
    int ready;
    ready = ezgrpc2_server_poll(server, &events, 400);
    if (ready < 0) {
      fprintf(stderr, "ezgrpc2_poll failed %d\n", ready);
      break;
    }
    for (int e = 0; e < ready; e++) {
      switch(events[e].event) {
        case EZGRPC2_EVENT_CLIENT_CONNECT:
        case EZGRPC2_EVENT_CLIENT_DCONNECT:
          continue;
        case EZGRPC2_EVENT_MESSAGE:
          if (ezgrpc2_workers_add_work(workers,((userdata_t*) events[e].message.userdata)->callback, NULL) < 0)
            abort();
      }
      ezgrpc2_event_free(events, ready);
    }
    int *wid;
    if ((ready = ezgrpc2_workers_poll(thpo)) > 0) {
      for (int i = 0; i < ready; i++)
        void *ret = ezgrpc2_workers_remove_work(workers, wid[i]);
    }
    
    if (r == 0)
      continue;

  }
  ezgrpc2_workers_cleanup(workers);
  ezgrpc2_server_cleanup(server);
  return 0;

}
