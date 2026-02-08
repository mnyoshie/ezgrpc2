#ifndef EZGRPC2_MESSAGES_H
#define EZGRPC2_MESSAGES_H

#include "ezgrpc2_list.h"	
#include "ezgrpc2_message.h"

/* an opaque type. Although ezgrpc2_message has no struct definition,
 * it's better this way so the compiler could inform better about type
 * mismatch.
 */
typedef struct ezgrpc2_messages ezgrpc2_messages;

static inline ezgrpc2_message *ezgrpc2_messages_pop(ezgrpc2_messages *messages) {
  return (ezgrpc2_message*)ezgrpc2_list_pop_front((ezgrpc2_list*)messages);
}

static inline int ezgrpc2_messages_push(ezgrpc2_messages *messages, ezgrpc2_message *message) {
  return ezgrpc2_list_push_back((ezgrpc2_list*)messages, message);
}

static inline ezgrpc2_messages *ezgrpc2_messages_new(void *unused) {
  return (ezgrpc2_messages*)ezgrpc2_list_new(unused);
}

static inline size_t ezgrpc2_messages_count(ezgrpc2_messages *messages) {
  return ezgrpc2_list_count((ezgrpc2_list*)messages);
}

static inline void ezgrpc2_messages_free(ezgrpc2_messages *messages) {
  if (messages == NULL) return;
  ezgrpc2_list *lmessages = (ezgrpc2_list*)messages;
  ezgrpc2_message *message;
  while ((message = ezgrpc2_list_pop_front(lmessages)) != NULL) {
    ezgrpc2_message_free(message);
  }

  ezgrpc2_list_free(lmessages);
}

#endif
