/* list.c - high performance doubly linked list */

#include <stdlib.h>
#include <assert.h>
#include "core.h"
#include "ezgrpc2_list.h"

typedef struct node_t node_t;
struct node_t {
  node_t *next;
  node_t *prev;
  void *data;
};

struct ezgrpc2_list_t {
  /* it's better to visualize this list as */
  /* front ---------------------> back */
  /* front->prev is always NULL and back next is always NULL */
  node_t *front;
  node_t *back;
  size_t count;
};

EZGRPC2_API ezgrpc2_list_t *ezgrpc2_list_new(void *unused) {
  ezgrpc2_list_t *ezlist = malloc(sizeof(*ezlist));
  ezlist->front = NULL;
  ezlist->back = NULL;
  ezlist->count = 0;
  return ezlist;
}

EZGRPC2_API void ezgrpc2_list_free(ezgrpc2_list_t *ezlist) {
  free(ezlist);
}

/* appends to the last element */
EZGRPC2_API int ezgrpc2_list_push_back(ezgrpc2_list_t *ezlist, void *data) {
  assert(ezlist != NULL);
  node_t *new_node = malloc(sizeof(*new_node));
  new_node->data = data;
  new_node->next = NULL;
  new_node->prev = NULL;
  if (ezlist->back != NULL) {
    ezlist->back->next = new_node;
    new_node->prev = ezlist->back;
    ezlist->back = new_node;
  } else {
    ezlist->front = new_node;
    ezlist->back = new_node;
  }

  ezlist->count++;

  return 0; 
}

/* inserts to the first element */
EZGRPC2_API int ezgrpc2_list_push_front(ezgrpc2_list_t *ezlist, void *data) {
  assert(ezlist != NULL);
  node_t *new_node = malloc(sizeof(*new_node));
  new_node->data = data;
  new_node->next = ezlist->front;
  new_node->prev = NULL;
  if (ezlist->front != NULL)
    ezlist->front->prev = new_node;
  else 
    ezlist->back = new_node;
  ezlist->front = new_node;

  ezlist->count++;
  return 0;
}

/* removes last element */
EZGRPC2_API void *ezgrpc2_list_pop_back(ezgrpc2_list_t *ezlist) {
  assert(ezlist != NULL);
  void *data;
  if (!ezlist->count)
    return NULL;

  node_t *back = ezlist->back;
  data = back->data;
  ezlist->back = back->prev;

  if (ezlist->back != NULL) {
    ezlist->back->next = NULL;
  } else {
    ezlist->front = NULL;
    assert(ezlist->count == 1);
  }

  ezlist->count--;
  return data;
}

/* pop front */
EZGRPC2_API void *ezgrpc2_list_pop_front(ezgrpc2_list_t *ezlist) {
  assert(ezlist != NULL);
  if (!ezlist->count)
    return NULL;
  void *data;

  node_t *front = ezlist->front;
  data = front->data;
  ezlist->front = front->next;

  if (ezlist->front != NULL) {
    ezlist->front->prev = NULL;
  } else {
    ezlist->back = NULL;
    assert(ezlist->count == 1);
  }

  ezlist->count--;
  return data;
}



EZGRPC2_API size_t ezgrpc2_list_count(ezgrpc2_list_t *ezlist) {
  return ezlist->count;
}

EZGRPC2_API int ezgrpc2_list_is_empty(ezgrpc2_list_t *ezlist) {
  return !ezlist->count;
}

EZGRPC2_API void *ezgrpc2_list_find(ezgrpc2_list_t *ezlist, int (*cmp)(const void *data, const void *userdata), void *userdata) {
  node_t *node = ezlist->front;
  while (node != NULL && cmp(node->data, userdata)) {
    node = node->next;
  }

  return node != NULL ? node->data : NULL;
}

/* remove */
EZGRPC2_API void *ezgrpc2_list_remove(ezgrpc2_list_t *ezlist, int (*cmp)(const void *data, const void *userdata), void *userdata) {
  node_t *node = ezlist->front;
  void *data = NULL;
  while (node != NULL && cmp(node->data, userdata)) {
    node = node->next;
  }
  if (node != NULL) {
    node_t *prev = node->prev;
    node_t *next = node->next;
    if (prev != NULL) {
      prev->next = next;
    } else {
      ezlist->front = next;
    }
    if (next != NULL) {
      next->prev = prev;
    } else {
      ezlist->back = prev;
    }
    data = node->data;
    free(node);
    ezlist->count--;
  }

  return data;
}


EZGRPC2_API void *ezgrpc2_list_peek_front(ezgrpc2_list_t *ezlist) {
  assert(ezlist != NULL);
  return ezlist->front != NULL ? ezlist->front->data: NULL;
}

EZGRPC2_API void ezgrpc2_list_concat_and_empty_src(ezgrpc2_list_t *dst, ezgrpc2_list_t *src) {
  if (!src->count)
    return;

  if (dst->front != NULL) {
  }
  else
    dst->front = src->front;
  if (dst->back != NULL) {
    dst->back->next = src->front;
    src->front->prev = dst->back;
  }
  else
    dst->back = src->back;
  dst->back = src->back;
  dst->count += src->count;

  src->front = NULL;
  src->back = NULL;
  src->count = 0;
}

EZGRPC2_API void ezgrpc2_list_foreach(ezgrpc2_list_t *ezlist, void (*func)(const void *data, const void *userdata), void *userdata) {
  for (node_t *n = ezlist->front; n != NULL; n = n->next)
    func(n->data, userdata);

}
//void ezgrpc2_list_print(ezgrpc2_list_t *l) {
//  for (listb_t **p = (listb_t**)&l->head; *p != NULL; p = &(*p)->next) {
//    printf("%p\n",  (*p)->data);
//  }
//}

#if 0

int cmp(void *data, void *userdata) {
  return data == userdata;
}

int main() {
  ezgrpc2_list_t l;
  ezgrpc2_list_init(&l);
  ezgrpc2_list_push_front(&l, (void*)0x123);
  ezgrpc2_list_push_front(&l, (void*)0x13);
  ezgrpc2_list_push_front(&l, (void*)0x23);
  ezgrpc2_list_popb(&l);
  ezgrpc2_list_push_front(&l, (void*)0x125);
  ezgrpc2_list_push_front(&l, (void*)0x925);
  ezgrpc2_list_popb(&l);
  ezgrpc2_list_popb(&l);
  ezgrpc2_list_push_front(&l, (void*)0x325);
  if (ezgrpc2_list_remove(&l, cmp, (void*)0x125) == NULL)
    puts("hi");
  ezgrpc2_list_push_front(&l, (void*)0x13);
  ezgrpc2_list_pushb(&l , (void*)0x535);

  ezgrpc2_list_print(&l);

  return 0;
}
#endif

