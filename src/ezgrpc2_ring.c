/* ezgrpc2_ring.c */

#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include "core.h"
#include "ezgrpc2_arena.h"

struct ezgrpc2_arena {
  /* indexes */
  size_t write;
  size_t read;
  /* number of bytes written */
  size_t count;
  /* size of buf */
  size_t size;
  uint8_t buf[];
};

EZGRPC2_API ezgrpc2_arena *ezgrpc2_arena_new(size_t size) {
  ezgrpc2_arena *arena = malloc(sizeof(*arena) + size);
  if (arena == NULL)
    return NULL;
  arena->read = 0;
  arena->write = 0;
  /* size of each elements */
  arena->size = size;
  return arena;
}

EZGRPC2_API void ezgrpc2_arena_free(ezgrpc2_arena *ezarena) {
  free(ezarena);
}

/* appends to the last element */
EZGRPC2_API void *ezgrpc2_arena_malloc(ezgrpc2_arena *arena, size_t size) {
  assert(arena != NULL);

  size_t alignment = (uintptr_t)(const void *)&arena->buf[arena->write] % alignof(max_align_t); 
  if (arena->write + size + alignment > arena->size)
    return NULL;
  
  arena->write = (arena->write + size + alignment);


  return &arena->buf[arena->write + alignment]; 
}


/* appends to the last element */
EZGRPC2_API void *ezgrpc2_arena_read(ezgrpc2_arena *arena, size_t size) {
  assert(arena != NULL);
  if (arena->count == 0)
    return NULL;
  void *ret = &arena->buf[arena->read*arena->size];
  arena->read = (arena->read + 1)%arena->depth;

  arena->count--;

  return ret; 
}

EZGRPC2_API int ezgrpc2_arena_is_empty(ezgrpc2_arena *arena) {
  return !arena->count;
}

EZGRPC2_API int ezgrpc2_arena_count(ezgrpc2_arena *arena) {
  return arena->count;
}

