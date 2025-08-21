#ifndef EZGRPC2_MESSAGE_H
#define EZGRPC2_MESSAGE_H
#include "common.h"

typedef struct ezgrpc2_message_t ezgrpc2_message_t;
/**
 * A gRPC message
 */
struct ezgrpc2_message_t {
  u8 is_compressed;
  u32 len;
  u8 *data;
};
#endif
