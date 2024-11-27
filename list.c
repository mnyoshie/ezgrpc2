/* list.c - high performance singly linked list */

#include "list.h"

typedef struct listb_t listb_t;
struct listb_t {
  void *data;
  listb_t *next;
};


void ezgrpc2_list_init(ezgrpc2_list_t *l) {
  l->head = NULL;
  l->tail = &l->head;
}

size_t ezgrpc2_list_count(ezgrpc2_list_t *l) {
  size_t c = 0;
  for (listb_t **b = (listb_t**)&l->head; *b != NULL; b = &(*b)->next)
    c++;

  return c;
}

int ezgrpc2_list_is_empty(ezgrpc2_list_t *l) {
  return l->head == NULL;
}

void *ezgrpc2_list_find(ezgrpc2_list_t *l, int (*cmp)(void *data, void *userdata), void *userdata) {
  for (listb_t **b = (listb_t**)&l->head; *b != NULL; b = &(*b)->next) {
    if (cmp((*b)->data, userdata))
      return (*b)->data;
  }
  return NULL;
}

/* remove */
void *ezgrpc2_list_remove(ezgrpc2_list_t *l, int (*cmp)(void *data, void *userdata), void *userdata) {
  listb_t **b = (listb_t**)&l->head;

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
int ezgrpc2_list_pushf(ezgrpc2_list_t *l, void *data){
  listb_t *b = malloc(sizeof(*b));
  if (b == NULL) {
    return 1;
  }
  b->data = data;
  b->next = NULL;

  *((listb_t**)l->tail) = b;
  l->tail = (listb_t**)&b->next;
  return 0;
}

int ezgrpc2_list_pushb(ezgrpc2_list_t *l, void *data){
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
void *ezgrpc2_list_popb(ezgrpc2_list_t *l) {
  if (l == NULL || l->head == NULL ) {
    return NULL;
  }

  listb_t *b = l->head;
  l->head = ((listb_t*)l->head)->next;
  if (l->head == NULL)
    l->tail = &l->head;

  void *ret = b->data;
  free(b);
  return ret;
}

/* (peek back) */
void *ezgrpc2_list_peekb(ezgrpc2_list_t *l) {
  if (l->head == NULL)
    return NULL;

  return ((listb_t*)l->head)->data;
}

void ezgrpc2_list_pushf_list(ezgrpc2_list_t *dst, ezgrpc2_list_t *src) {
  if (dst->head == NULL)
    dst->head = src->head;
  *(listb_t**)dst->tail = src->head;
  dst->tail = src->tail;
  ezgrpc2_list_init(src);
}


void ezgrpc2_list_print(ezgrpc2_list_t *l) {
  for (listb_t **p = (listb_t**)&l->head; *p != NULL; p = &(*p)->next) {
    printf("%p\n",  (*p)->data);
  }
}

#if 0

int cmp(void *data, void *userdata) {
  return data == userdata;
}

int main() {
  ezgrpc2_list_t l;
  ezgrpc2_list_init(&l);
  ezgrpc2_list_pushf(&l, (void*)0x123);
  ezgrpc2_list_pushf(&l, (void*)0x13);
  ezgrpc2_list_pushf(&l, (void*)0x23);
  ezgrpc2_list_popb(&l);
  ezgrpc2_list_pushf(&l, (void*)0x125);
  ezgrpc2_list_pushf(&l, (void*)0x925);
  ezgrpc2_list_popb(&l);
  ezgrpc2_list_popb(&l);
  ezgrpc2_list_pushf(&l, (void*)0x325);
  if (ezgrpc2_list_remove(&l, cmp, (void*)0x125) == NULL)
    puts("hi");
  ezgrpc2_list_pushf(&l, (void*)0x13);
  ezgrpc2_list_pushb(&l , (void*)0x535);

  ezgrpc2_list_print(&l);

  return 0;
}
#endif

