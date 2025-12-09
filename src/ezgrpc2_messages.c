#include "ezgrpc2_messages.h"
#include "common.h"

#ifdef _WIN32
#  include <ws2tcpip.h>
#else /* __unix__ */
#  include <arpa/inet.h>
#endif

typedef struct ezgrpc2_messages ezgrpc2_messages;
struct ezgrpc2_messages {
  u8 *data;
  size_t data_len;
  size_t nb_messages;
  size_t seek;
};

static size_t is_valid_length_prefixed_messages(void *data, size_t len, size_t *nb_messages) {
  if (!len)
    return 0;

  i8 *wire = data;
  size_t seek = 0;
  while (seek < len) {
    if (seek + 1 > len)
      return 0;
    seek += 1;
    if (seek + 4 > len)
      return 0;
    u32 msg_len = ntohl(uread_u32(wire + seek));
    seek += 4;
    if (seek + msg_len > len)
      return 0;
    seek += msg_len;
    (*nb_messages)++;
  }
  return seek;
}

/* NOTE: expect message->data is unaligned. */
ezgrpc2_message ezgrpc2_messages_peek(ezgrpc2_messages *messages) {
  if (messages->seek == messages->data_len)
    return (ezgrpc2_message) {
      .compressed_flag = 0,
      .len = 0,
      .data = NULL
    };
  return (ezgrpc2_message){
    .compressed_flag = messages->data[messages->seek],
    .len = htonl(uread_u32(messages->data + messages->seek + 1)),
    .data = messages->data + messages->seek + 4
  };
}

void ezgrpc2_messages_rewind(ezgrpc2_messages *messages) {
  messages->seek = 0;
}

size_t ezgrpc2_messages_count(ezgrpc2_messages *messages) {
  return messages->nb_messages;
}

ezgrpc2_message ezgrpc2_messages_next(ezgrpc2_messages *messages) {
  if (messages->seek == messages->data_len)
    return (ezgrpc2_message) {
      .compressed_flag = 0,
      .len = 0,
      .data = NULL
    };
  char compressed_flag = messages->data[messages->seek++];
  size_t len = htonl(uread_u32((u8*)messages->data + messages->seek));
  messages->seek += 4;
  size_t pseek = messages->seek;


  return messages->seek += len, (ezgrpc2_message){
    .compressed_flag = compressed_flag,
    .len = len,
    .data = messages->data + pseek 
  };
}

ezgrpc2_messages *ezgrpc2_messages_new(void *data, size_t len){
  size_t nb_messages = 0;  
  if (!is_valid_length_prefixed_messages(data, len, &nb_messages))
    return NULL;
  ezgrpc2_messages *messages = malloc(sizeof(*messages));
  messages->data_len = len;
  messages->data = data;
  messages->seek = 0;
  messages->nb_messages = nb_messages;
  return messages;
}

