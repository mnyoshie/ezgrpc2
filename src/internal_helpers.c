#include <assert.h>
#include "core.h"
#include "ezgrpc2_header.h"
#include "internal_helpers.h"
#define atlog(...) (void)0
/* creates a ezstream and adds it to the linked list */
__attribute__((visibility("hidden"))) ezgrpc2_stream_t *stream_new(i32 stream_id) {
  ezgrpc2_stream_t *ezstream = calloc(1, sizeof(*ezstream));
  ezstream->lqueue_omessages = ezgrpc2_list_new(NULL);
  ezstream->lheaders = ezgrpc2_list_new(NULL);
  ezstream->time = (uint64_t)time(NULL);
  ezstream->is_trunc = 0;
  ezstream->trunc_seek = 0;
  ezstream->stream_id = stream_id;
  return ezstream;
}
/* only frees what is passed. it does not free all the linked lists */
__attribute__((visibility("hidden"))) void stream_free(ezgrpc2_stream_t *ezstream) {
  atlog("stream %d freed\n", ezstream->stream_id);
  ezgrpc2_header_t *ezheader;

  while ((ezheader = ezgrpc2_list_pop_front(ezstream->lheaders)) != NULL) {
    ezgrpc2_header_free(ezheader);
  }
  free(ezstream->recv_data);
  ezgrpc2_list_free(ezstream->lheaders);
  free(ezstream);
}

// __attribute__((visibility("hidden"))) ezgrpc2_header_t *header_create(const u8 *name, size_t nlen, const u8 *value, size_t vlen) {
//  ezgrpc2_header_t *ezheader = malloc(sizeof(*ezheader));
//  if (ezheader == NULL)
//    return NULL;
//
//  ezheader->name = malloc(nlen);
//  ezheader->value = malloc(vlen);
//
//  memcpy(ezheader->name, name, nlen);
//  memcpy(ezheader->value, value, vlen);
//  ezheader->nlen = nlen;
//  ezheader->vlen = vlen;
//
//  return ezheader;
//}
//
//__attribute__((visibility("hidden"))) void header_free(ezgrpc2_header_t *ezheader) {
//  free(ezheader->name);
//  free(ezheader->value);
//}
