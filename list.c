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

/* remove */
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

/* appends to the end (tail) (push front)*/
int list_pushf(list_t *l, void *data){
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

int list_pushb(list_t *l, void *data){
  listb_t *b = malloc(sizeof(*b));
  if (b == NULL) {
    return 1;
  }
  b->data = data;
  b->next = l->head;

  l->head = b;
  return 0;
}

/* pops the head (pop back) */
void *list_popb(list_t *l) {
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

/* (peek back) */
void *list_peekb(list_t *l) {
  if (l->head == NULL)
    return NULL;

  return l->head->data;
}

int list_pushf_list(list_t *dst, list_t *src) {
  if (dst->head == NULL)
    dst->head = src->head;
  *dst->tail = src->head;
  dst->tail = src->tail;
  list_init(src);
  return 0;

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

int main() {
  list_t l;
  list_init(&l);
  list_pushf(&l, (void*)0x123);
  list_pushf(&l, (void*)0x13);
  list_pushf(&l, (void*)0x23);
  list_popb(&l);
  list_pushf(&l, (void*)0x125);
  list_pushf(&l, (void*)0x925);
  list_popb(&l);
  list_popb(&l);
  list_pushf(&l, (void*)0x325);
  if (list_remove(&l, cmp, (void*)0x125) == NULL)
    puts("hi");
  list_pushf(&l, (void*)0x13);
  list_pushb(&l , (void*)0x535);

  list_print(&l);

  return 0;
}
#endif

