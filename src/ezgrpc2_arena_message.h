#ifndef EZGRPC2_ARENA_MESSAGE_H
#define EZGRPC2_ARENA_MESSAGE_H

#include <stdlib.h>
#include "ezgrpc2_message.h"

typedef struct ezgrpc2_arena_message ezgrpc2_arena_message;

EZGRPC2_API ezgrpc2_arena_message *ezgrpc2_arena_message_new(size_t size);

EZGRPC2_API void ezgrpc2_arena_message_free(ezgrpc2_arena_message *arena_message);

/* appends to the last element */
EZGRPC2_API ezgrpc2_message *ezgrpc2_arena_message_malloc(ezgrpc2_arena_message *arena_message, size_t size);


/* appends to the last element */
EZGRPC2_API ezgrpc2_message *ezgrpc2_arena_message_read(ezgrpc2_arena_message *arena_message);

EZGRPC2_API size_t ezgrpc2_arena_message_is_empty(ezgrpc2_arena_message *arena_message);

EZGRPC2_API int ezgrpc2_arena_message_count(ezgrpc2_arena_message *arena_message);

#endif
