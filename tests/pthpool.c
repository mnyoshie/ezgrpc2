#include <stdlib.h>
#include <unistd.h>
#include "pthpool.h"
#include "list.h"
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
  pthpool_t *pool = pthpool_init(4, -1);
  while (1) {
    pthpool_add_task(pool, callback, NULL, NULL, NULL);
    printf("task\n");
    list_t l;
    pthpool_poll(pool, &l);
    task_t *t;
    while ((t = list_pop(&l)) != NULL) {
      printf("popped\n");
      free(t);
    }
  }
  pthpool_destroy(pool);
}
