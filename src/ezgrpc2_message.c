#include <stdlib.h>
#include <assert.h>
#include "ezgrpc2_message.h"

ezgrpc2_message_t *ezgrpc2_message_new(size_t data_len) {
  ezgrpc2_message_t *message = malloc(sizeof(*message));
  assert(message != NULL);
  message->data = malloc(data_len);
  assert(message->data != NULL);
  message->len = data_len;
  return message;
}

void ezgrpc2_message_free(ezgrpc2_message_t *message) {
  free(message->data);
  free(message);
}
