#ifndef EZGRPC2_RING_H
#define EZGRPC2_RING_H
#include <stdlib.h>



/**
 * An opaque ring context. 
 *
 */
typedef struct ezgrpc2_ring ezgrpc2_ring;


EZGRPC2_API ezgrpc2_ring *ezgrpc2_ring_new(size_t depth, size_t size);
EZGRPC2_API void ezgrpc2_ring_free(ezgrpc2_ring *ring);
EZGRPC2_API int ezgrpc2_ring_write(ezgrpc2_ring *ring, void *data);

/* appends to the last element */
EZGRPC2_API void *ezgrpc2_ring_read(ezgrpc2_ring *ring);










#endif
