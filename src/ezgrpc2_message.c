#include <stdlib.h>
#include <assert.h>
#include "core.h"
#include "ezgrpc2_message.h"

EZGRPC2_API ezgrpc2_message *ezgrpc2_message_new(uint8_t is_compressed, void *data, size_t data_len) {
  ezgrpc2_message *message = malloc(sizeof(*message) + data_len);
  assert(message != NULL);
  memcpy(message->data, data, data_len);
  message->len = data_len;
  return message;
}

//EZGRPC2_API ezgrpc2_message *ezgrpc2_message_new2(uint8_t is_compressed, void *data, size_t data_len) {
//  ezgrpc2_message *message = ezgrpc2_message_new(data_len);
//  assert(message != NULL);
//  memcpy(message->data, data, data_len);
//  return message;
//}

EZGRPC2_API void ezgrpc2_message_free(ezgrpc2_message *message) {
  free(message);
}
