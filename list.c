#include "list.h"

struct listb_t {
  void *data;
  listb_t *next;
};


void list_init(list_t *l) {
  l->head = NULL;
  l->tail = &l->head;
}

size_t list_count(list_t *l) {
  size_t c = 0;
  for (listb_t **b = &l->head; *b != NULL; b = &(*b)->next)
    c++;

  return c;
}

int list_is_empty(list_t *l) {
  return l->head == NULL;
}

void *list_find(list_t *l, int (*cmp)(void *data, void *userdata), void *userdata) {
  for (listb_t **b = &l->head; *b != NULL; b = &(*b)->next) {
    if (cmp((*b)->data, userdata))
      return (*b)->data;
  }
  return NULL;
}

void *list_remove(list_t *l, int (*cmp)(void *data, void *userdata), void *userdata) {
  listb_t **b = &l->head;

  while (*b != NULL && !cmp((*b)->data, userdata))
    b = &(*b)->next;

  if (*b != NULL) {
    void *ret = (*b)->data;
    listb_t *n = (*b)->next;
    free(*b);
    *b = n;
    if (*b == NULL)
      l->tail = b;
    return ret;
  }
  return NULL;
}

/* appends to the end (tail) */
int list_add(list_t *l, void *data){
  listb_t *b = malloc(sizeof(*b));
  if (b == NULL) {
    return 1;
  }
  b->data = data;
  b->next = NULL;

  *(l->tail) = b;
  l->tail = &b->next;
  return 0;
}

/* pops the head */
void *list_pop(list_t *l) {
  if (l == NULL || l->head == NULL ) {
    return NULL;
  }

  listb_t *b = l->head;
  l->head = l->head->next;
  if (l->head == NULL)
    l->tail = &l->head;

  void *ret = b->data;
  free(b);
  return ret;
}

void *list_peek(list_t *l) {
  if (l->head == NULL)
    return NULL;

  return l->head->data;
}

void list_print(list_t *l) {
  for (listb_t **p = &l->head; *p != NULL; p = &(*p)->next) {
    printf("%p\n",  (*p)->data);
  }
}

#if 0

int cmp(void *data, void *userdata) {
  return data == userdata;
}

int main0() {
  list_t l;
  list_init(&l);
  list_add(&l, (void*)0x123);
  list_add(&l, (void*)0x13);
  list_add(&l, (void*)0x23);
  list_pop(&l);
  list_add(&l, (void*)0x125);
  list_add(&l, (void*)0x925);
  list_pop(&l);
  list_pop(&l);
  list_add(&l, (void*)0x325);
  if (list_remove(&l, cmp, (void*)0x125) == NULL)
    puts("hi");
  list_add(&l, (void*)0x13);

  list_print(&l);

  return 0;
}
#endif

