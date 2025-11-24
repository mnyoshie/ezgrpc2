#ifndef EZGRPC2_MESSAGE_H
#define EZGRPC2_MESSAGE_H
#include "common.h"

typedef struct ezgrpc2_message ezgrpc2_message;
/**
 * A gRPC message
 */
struct ezgrpc2_message {
  uint8_t is_compressed;
  uint32_t len;
  uint8_t data[];
};

ezgrpc2_message *ezgrpc2_message_new(uint8_t is_compressed, void *data, size_t data_len);
//ezgrpc2_message *ezgrpc2_message_new2(void *data, size_t data_len);
void ezgrpc2_message_free(ezgrpc2_message *message);
#endif
