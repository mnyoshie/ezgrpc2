#include "common.h"
#include "ezgrpc2_header.h"

ezgrpc2_header_t *ezgrpc2_header_new(const u8 *name, size_t nlen, const u8 *value, size_t vlen) {
  ezgrpc2_header_t *ezheader = malloc(sizeof(*ezheader));
  if (ezheader == NULL)
    return NULL;

  ezheader->name = malloc(nlen);
  ezheader->value = malloc(vlen);

  memcpy(ezheader->name, name, nlen);
  memcpy(ezheader->value, value, vlen);
  ezheader->nlen = nlen;
  ezheader->vlen = vlen;

  return ezheader;
}

void ezgrpc2_header_free(ezgrpc2_header_t *ezheader) {
  free(ezheader->name);
  free(ezheader->value);
}
