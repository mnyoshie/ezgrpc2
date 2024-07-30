#ifndef MNLIST_H
#define MNLIST_H
#include <stdlib.h>


typedef struct listb_t listb_t;
struct listb_t;

typedef struct list_t list_t;
struct list_t {
  listb_t *head, **tail;
};

void list_init(list_t *);
size_t list_count(list_t *);
int list_pushf(list_t *, void *);
void *list_popb(list_t *);
void list_print(list_t *);

void *list_find(list_t *l, int (*cmp)(void *data, void *userdata), void *userdata);
void *list_remove(list_t *l, int (*cmp)(void *data, void *userdata), void *userdata);
int list_is_empty(list_t *l);
void *list_peekb(list_t *l);
#endif
