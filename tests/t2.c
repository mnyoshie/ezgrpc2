#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "ezgrpc2.h"
#include "thpool.h"
void ret_cleanup(void *p) {
  puts("called ret_cleanup");
  free(p);
}
void usrdata_cleanup(void *p) {
  puts("called usrdata_cleanup");
  free(p);
}
void *callback(void *data){return NULL;}


int main() {
#if 0
  thpool_t *pool = thpool_init(4, -1);
  while (1) {
    thpool_add_task(pool, callback, NULL, NULL, NULL);
    printf("task\n");
    list_t *l = thpool_poll(pool);
    task_t *t;
    while ((t = list_pop(l)) != NULL) {
      printf("popped\n");
      free(t);
    }
    free(l);
  }
  thpool_destroy(pool);
#endif
  //ezgrpc2_server_t *server = ezgrpc2_server_init("127.0.0.1", 19009, NULL, -1, 16);
  ezgrpc2_server_t *server = ezgrpc2_server_init(NULL, -1, "::", 19009, 16);

  ezgrpc2_path_t paths[2] = {0};
  paths[0].path = "/test.yourAPI/whatever_service1";
  paths[1].path = "/test.yourAPI/someservice";
  list_init(&paths[0].list_events);
  list_init(&paths[1].list_events);
  int res;

  while (1) {
    res = ezgrpc2_server_poll(server, paths, 1, 10000);
    if (res > 0) {
      ezgrpc2_event_t *event;
      while ((event = list_pop(&paths[0].list_events)) != NULL) {
        switch(event->type) {
          case EZGRPC2_EVENT_MESSAGE:
            printf("event message %zu end stream %d\n", list_count(&event->message.list_messages), event->message.end_stream);
            //list_t list_messages = event->message.list_messages;
            list_init(&event->message.list_messages);
            list_t list_messages;
            list_init(&list_messages);
           
            for (int i = 0; i < 5; i++) {
              ezgrpc2_message_t *msg = malloc(sizeof(*msg));
              msg->is_compressed = 0;
              msg->data = malloc(32 + i);
              strcpy(msg->data, "Hello world!!");
              msg->len = 32 + i;
              list_add(&list_messages, msg);
            }
            ezgrpc2_session_send(event->ezsession, event->message.stream_id, &list_messages);
            if (event->message.end_stream)
              ezgrpc2_session_end_stream(event->ezsession, event->message.stream_id, 0);
            break;
          case EZGRPC2_EVENT_DATALOSS:
            printf("event dataloss\n");
            break;
          case EZGRPC2_EVENT_CANCEL:
            printf("event message\n");
            break;
        }
        free(event);
      }



      continue;
    }
    else if (res == 0)
      printf("no event\n");
    else if (res < 0)
      printf("here error\n");
  }
  assert(server != NULL);
  ezgrpc2_server_destroy(server);
}
