#include <stdlib.h>
#include <unistd.h>
#include "thpool.h"
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
  thpool_t *pool = thpool_init(4, -1);
  while (1) {
    thpool_add_task(pool, callback, NULL, NULL, NULL);
    printf("task\n");
    list_t l;
    thpool_poll(pool, &l);
    task_t *t;
    while ((t = list_pop(&l)) != NULL) {
      printf("popped\n");
      free(t);
    }
  }
  thpool_destroy(pool);
}
