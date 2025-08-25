#ifndef EZGRPC2_INTERNAL_HELPERS_H
#define EZGRPC2_INTERNAL_HELPERS_H

#include "core.h"

void stream_free(ezgrpc2_stream_t *stream);
ezgrpc2_stream_t *stream_new(i32 stream_id);
//
//ezgrpc2_header_t *header_create(const u8 *name, size_t nlen, const u8 *value, size_t vlen);
//
//void header_free(ezgrpc2_header_t *ezheader);
#endif
