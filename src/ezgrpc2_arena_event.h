#ifndef EZGRPC2_ARENA_EVENT_H
#define EZGRPC2_ARENA_EVENT_H

#include "ezgrpc2_event.h"

typedef struct ezgrpc2_arena_event ezgrpc2_arena_event;

EZGRPC2_API ezgrpc2_arena_event *ezgrpc2_arena_event_new(size_t size);

EZGRPC2_API void ezgrpc2_arena_event_free(ezgrpc2_arena_event *arena_event);

/* appends to the last element */
EZGRPC2_API ezgrpc2_event *ezgrpc2_arena_event_malloc(ezgrpc2_arena_event *arena_event);


EZGRPC2_API ezgrpc2_event *ezgrpc2_arena_event_read(ezgrpc2_arena_event *arena_event);

EZGRPC2_API int ezgrpc2_arena_event_is_empty(ezgrpc2_arena_event *arena_event);

size_t ezgrpc2_arena_event_count(ezgrpc2_arena_event *arena_event);

EZGRPC2_API void ezgrpc2_arena_event_reset(ezgrpc2_arena_event *arena_event);

#endif /* EZGRPC2_ARENA_EVENT_H */
