/* ezgrpc2_ring.c */

#include <stdlib.h>
#include <assert.h>
#include "core.h"
#include "ezgrpc2_ring.h"

struct ezgrpc2_ring {
  /* indexes */
  size_t write;
  size_t read;
  /* number of elements written */
  size_t count;
  /* number of elements that can be written */
  size_t depth;
  /* size of each element */
  size_t size;
  uint8_t buf[];
};

EZGRPC2_API ezgrpc2_ring *ezgrpc2_ring_new(size_t depth, size_t size) {
  ezgrpc2_ring *ring = malloc(sizeof(*ring) + sizeof(uint8_t)*depth*size);
  if (ring == NULL)
    return NULL;
  ring->read = 0;
  ring->write = 0;
  ring->count = 0;
  /* number of elements */
  ring->depth = depth;
  /* size of each elements */
  ring->size = size;
  return ring;
}

EZGRPC2_API void ezgrpc2_ring_free(ezgrpc2_ring *ezring) {
  free(ezring);
}

/* appends to the last element */
EZGRPC2_API int ezgrpc2_ring_write(ezgrpc2_ring *ring, void *data) {
  assert(ring != NULL);
  if (ring->count + 1 > ring->depth)
    return 1;
  memcpy(&ring->buf[ring->write*ring->size], data, ring->size);;
  ring->write = (ring->write + 1)%ring->depth;

  ring->count++;

  return 0; 
}
/* appends to the last element */
EZGRPC2_API void *ezgrpc2_ring_read(ezgrpc2_ring *ring, void *data) {
  assert(ring != NULL);
  if (ring->count == 0)
    return NULL;
  void *ret = &ring->buf[ring->read*ring->size];
  ring->read = (ring->read + 1)%ring->depth;

  ring->count--;

  return ret; 
}

EZGRPC2_API int ezgrpc2_ring_is_empty(ezgrpc2_ring *ring) {
  return !ring->count;
}

EZGRPC2_API int ezgrpc2_ring_count(ezgrpc2_ring *ring) {
  return ring->count;
}

