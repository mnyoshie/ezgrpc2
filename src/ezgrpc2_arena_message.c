/* ezgrpc2_ring.c */

#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include "ezgrpc2_message.h"
#include "ezgrpc2_arena_message.h"

struct ezgrpc2_arena_message {
  /* indexes */
  size_t write;
  size_t read;
  size_t count;
  /* size of buf */
  size_t size;
  uint8_t buf[];
};

EZGRPC2_API ezgrpc2_arena_message *ezgrpc2_arena_message_new(size_t size) {
  ezgrpc2_arena_message *arena_message = malloc(sizeof(*arena_message) + size);
  if (arena_message == NULL)
    return NULL;
  arena_message->read = 0;
  arena_message->write = 0;
  arena_message->count = 0;
  /* size of each elements */
  arena_message->size = size;
  return arena_message;
}

EZGRPC2_API void ezgrpc2_arena_message_free(ezgrpc2_arena_message *arena_message) {
  free(arena_message);
}

/* appends to the last element */
EZGRPC2_API ezgrpc2_message *ezgrpc2_arena_message_malloc(ezgrpc2_arena_message *arena_message, size_t size) {
  assert(arena_message != NULL);

  /*  ensure the memory we're returning is aligned */
  const size_t alignment = (uintptr_t)(const void *)&arena_message->buf[arena_message->write] % alignof(ezgrpc2_message); 
  if (arena_message->write + sizeof(ezgrpc2_message) + size + alignment > arena_message->size)
    return NULL;
 
  ezgrpc2_message *msg = (void *)&arena_message->buf[alignment + arena_message->write]; 
  arena_message->write += alignment + sizeof(ezgrpc2_message) + size;


  msg->len = size;
  return msg;
}


/* appends to the last element */
EZGRPC2_API ezgrpc2_message *ezgrpc2_arena_message_read(ezgrpc2_arena_message *arena_message) {
  assert(arena_message != NULL);
  if (!arena_message->count)
    return NULL;

  const size_t alignment = (uintptr_t)(const void *)&arena_message->buf[arena_message->read] % alignof(ezgrpc2_message); 
  ezgrpc2_message *msg = (void *)&arena_message->buf[alignment + arena_message->read];
  arena_message->read += alignment + sizeof(ezgrpc2_message) + msg->len;
  arena_message->count--;

  return msg;
}

EZGRPC2_API size_t ezgrpc2_arena_message_is_empty(ezgrpc2_arena_message *arena_message) {
  return arena_message->count;
}

EZGRPC2_API int ezgrpc2_arena_message_count(ezgrpc2_arena_message *arena_message) {
  return arena_message->count;
}

