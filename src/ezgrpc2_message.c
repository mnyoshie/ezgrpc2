#include <stdlib.h>
#include <assert.h>
#include "core.h"
#include "ezgrpc2_message.h"

EZGRPC2_API ezgrpc2_message_t *ezgrpc2_message_new(size_t data_len) {
  ezgrpc2_message_t *message = malloc(sizeof(*message));
  assert(message != NULL);
  message->data = malloc(data_len);
  assert(message->data != NULL);
  message->len = data_len;
  return message;
}

EZGRPC2_API ezgrpc2_message_t *ezgrpc2_message_new2(void *data, size_t data_len) {
  ezgrpc2_message_t *message = malloc(sizeof(*message));
  assert(message != NULL);
  message->data = malloc(data_len);
  assert(message->data != NULL);
  memcpy(message->data, data, data_len);
  message->len = data_len;
  return message;
}

EZGRPC2_API void ezgrpc2_message_free(ezgrpc2_message_t *message) {
  free(message->data);
  free(message);
}
