#include <stdlib.h>

static inline void *ezgrpc2_malloc(size_t size){
  return malloc(size);
}

static inline void ezgrpc2_free(void *ptr) {
  free(ptr);
}
