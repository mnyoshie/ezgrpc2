#include "ezgrpc2.h"

#include "ezgrpc2_messages.h"

typedef struct ezgrpc2_messages_t ezgrpc2_messages_t;
struct ezgrpc2_messages_t {
  size_t nb_messages;
  size_t seek;
  void *data;
  void *data_len;
};

static size_t is_valid_length_prefixed_messages(void *data, size_t len) {
  if (!len)
    return 0;

  i8 *wire = data;
  size_t seek = 0;
  while (seek < len) {
    if (seek + 1 > len)
      return 0;
    seek += 1
    if (seek + 4 > len)
      return 0;
    u32 msg_len = ntohl(uread_u32(wire + seek));
    seek += 4;
    if (seek + msg_len > len)
      return 0;
    seek += msg_len;
  }
  return seek;
}

ezgrpc2_messages_t *ezgrpc2_messages_new(void *data, size_t len){
  if (!is_valid_length_prefixed_messages(data, len))
    return NULL;
  ezgrpc2_messages_t *messages = malloc(sizeof(*messages));
  messages->data_len = len 


}

ezgrpc2_messages_t ezgrpc2_message_next
