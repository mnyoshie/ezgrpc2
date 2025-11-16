#ifndef EZGRPC2_MESSAGES_H
#define EZGRPC2_MESSAGES_H
#include <stdlib.h>
#include "ezgrpc2_message.h"

typedef struct ezgrpc2_messages ezgrpc2_messages;

/* NOTE: expect message->data is unaligned. */
ezgrpc2_message ezgrpc2_messages_peek(ezgrpc2_messages *messages);
void ezgrpc2_messages_rewind(ezgrpc2_messages *messages);
size_t ezgrpc2_messages_count(ezgrpc2_messages *messages);
ezgrpc2_message ezgrpc2_messages_next(ezgrpc2_messages *messages);
ezgrpc2_messages *ezgrpc2_messages_new(void *data, size_t len);

#endif
