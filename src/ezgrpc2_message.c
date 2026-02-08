#include <stdlib.h>
#include <assert.h>
#include "core.h"
#include "ezgrpc2_message.h"


EZGRPC2_API ezgrpc2_message *ezgrpc2_message_new(uint8_t cflag, void *data, size_t data_len) {
  size_t size = sizeof(ezgrpc2_message) + data_len;
  ezgrpc2_message *message = aligned_alloc(CONFIG_CACHE_LINE_SIZE, REALIGN(CONFIG_CACHE_LINE_SIZE, size));
  if (message == NULL)
    return NULL;
  message->cflag = cflag;
  memcpy(message->data, data, data_len);
  message->len = data_len;
  return message;
}

EZGRPC2_API ezgrpc2_message *ezgrpc2_message_new2(size_t data_len) {
  size_t size = sizeof(ezgrpc2_message) + data_len;
  ezgrpc2_message *message = aligned_alloc(CONFIG_CACHE_LINE_SIZE, REALIGN(CONFIG_CACHE_LINE_SIZE, size));
  return message;
}

EZGRPC2_API void ezgrpc2_message_free(ezgrpc2_message *message) {
  free(message);
}
