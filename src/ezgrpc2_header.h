#ifndef EZGRPC2_HEADER_H
#define EZGRPC2_HEADER_H
#include <stdlib.h>

typedef struct ezgrpc2_header_t ezgrpc2_header_t;

/**
 *
 */
struct ezgrpc2_header_t {
  size_t nlen;
  char *name;
  char *value;
  size_t vlen;
};

ezgrpc2_header_t *ezgrpc2_header_new(const u8 *name, size_t nlen, const u8 *value, size_t vlen);

void ezgrpc2_header_free(ezgrpc2_header_t *ezheader);
#endif
