#ifndef EZGRPC2_MESSAGES_H
#define EZGRPC2_MESSAGES_H
#include <stdlib.h>
#include "ezgrpc2_message.h"

typedef struct ezgrpc2_messages_t ezgrpc2_messages_t;

/* NOTE: expect message->data is unaligned. */
ezgrpc2_message_t ezgrpc2_messages_peek(ezgrpc2_messages_t *messages);
void ezgrpc2_messages_rewind(ezgrpc2_messages_t *messages);
size_t ezgrpc2_messages_count(ezgrpc2_messages_t *messages);
ezgrpc2_message_t ezgrpc2_messages_next(ezgrpc2_messages_t *messages);
ezgrpc2_messages_t *ezgrpc2_messages_new(void *data, size_t len);

#endif
