#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "ezgrpc2.h"
#include "pthpool.h"

void ret_cleanup(void *p) {
  puts("called ret_cleanup");
  free(p);
}
void usrdata_cleanup(void *p) {
  puts("called usrdata_cleanup");
  free(p);
}

struct userdata_t {
  uint8_t session_id[32];
  i32 stream_id;
  char end_stream;
  list_t list_imessages, list_omessages;
};

struct userdata_t *create_userdata(uint8_t session_id[32], int32_t stream_id,
    list_t list_messages, char end_stream) {
  struct userdata_t *data = malloc(sizeof(*data));
  memcpy(data->session_id, session_id, 32);
  data->stream_id = stream_id;
  data->list_imessages = list_messages;
  data->end_stream = end_stream;
  return data;
}

int pp = 0;
void *callback(void *data){
  struct userdata_t *usrdata = data;
  list_t *list_imessages = &usrdata->list_imessages;
  {
    ezgrpc2_message_t *msg;
    while ((msg = list_pop(list_imessages)) != NULL) {
      free(msg->data);
      free(msg);
    }
  }

  list_init(&usrdata->list_omessages);
  
  for (int i = 0; i < 5; i++, pp++) {
    ezgrpc2_message_t *msg = malloc(sizeof(*msg));
    msg->is_compressed = 0;
    msg->data = strdup("    Hello world!!");
    
    msg->len = strlen(msg->data) + 1;
    *(uint32_t*)(msg->data) = pp;
    list_add(&usrdata->list_omessages, msg);
  }
  printf("task executed!!\n");
  sleep(5);
  return data;
}


int main() {
  pthpool_t *pool = pthpool_init(1, -1);
#if 0
  while (1) {
    printf("task\n");
    list_t *l = thpool_poll(pool);
    task_t *t;
    while ((t = list_pop(l)) != NULL) {
      printf("popped\n");
      free(t);
    }
    free(l);
  }
#endif
  //ezgrpc2_server_t *server = ezgrpc2_server_init("127.0.0.1", 19009, NULL, -1, 16);
  ezgrpc2_server_t *server = ezgrpc2_server_init(NULL, -1, "::", 19009, 16);
  assert(server != NULL);

  ezgrpc2_path_t paths[2] = {0};
  paths[0].path = "/test.yourAPI/whatever_service1";
  paths[1].path = "/test.yourAPI/someservice";
  list_init(&paths[0].list_events);
  list_init(&paths[1].list_events);
  int res;

  while (1) {
    res = ezgrpc2_server_poll(server, paths, 1, 5);
    if (res > 0) {
      ezgrpc2_event_t *event;
      while ((event = list_pop(&paths[0].list_events)) != NULL) {
        switch(event->type) {
          case EZGRPC2_EVENT_MESSAGE:
            printf("event message %zu end stream %d\n", list_count(&event->message.list_messages), event->message.end_stream);
            struct userdata_t *data = create_userdata(
                event->session_id, event->message.stream_id,
                event->message.list_messages,
                event->message.end_stream);
            /* clear */
            list_init(&event->message.list_messages);

            pthpool_add_task(pool, callback, data, NULL, NULL);
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
    }
    else if (res < 0) {
      printf("here error\n");
      assert(0);
    }
    //printf("heredjndjd\n");
    list_t list_tasks;
    pthpool_poll(pool, &list_tasks);
    task_t *task;
    while ((task = list_pop(&list_tasks)) != NULL) {
      struct userdata_t *data = task->ret;
      ezgrpc2_session_send(server, data->session_id, data->stream_id, &data->list_omessages);
      if (data->end_stream)
        ezgrpc2_session_end_stream(server, data->session_id, data->stream_id, 0);
      free(task->ret);
      free(task);
    }
  }
  ezgrpc2_server_destroy(server);
  pthpool_destroy(pool);
}
