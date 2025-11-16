#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include "ezgrpc2_list.h"
#include "ezgrpc2_pthpool.h"

void ret_cleanup(void *p) {
  puts("called ret_cleanup");
  free(p);
}
void usrdata_cleanup(void *p) {
  puts("called usrdata_cleanup");
  free(p);
}
void *callback(void *data){return NULL;};


int main() {
  ezgrpc2_pthpool *pool = ezgrpc2_pthpool_new(4, 0);
  for (int i = 0; i < 1000; i++) {
    ezgrpc2_pthpool_add_task(pool, callback, NULL, NULL, NULL);
    printf("task\n");
    ezgrpc2_list *l = ezgrpc2_list_new(NULL);
    ezgrpc2_pthpool_poll(pool, l);
    ezgrpc2_pthpool_result_t *t;
    while ((t = ezgrpc2_list_pop_front(l)) != NULL) {
      printf("popped\n");
      free(t);
    }
  }
  ezgrpc2_pthpool_stop_and_join(pool);
  ezgrpc2_pthpool_free(pool);
  return 0;
}
