#ifndef EZGRPC2_EVENTS_H
#define EZGRPC2_EVENTS_H

#include "ezgrpc2_event.h"

#define EZGRPC2_EVENTS_BLOCK (64)

typedef struct ezgrpc2_events ezgrpc2_events;



ezgrpc2_event *ezgrpc2_events_read(ezgrpc2_events *events, size_t *len);

ezgrpc2_events *ezgrpc2_events_new(void *unused);

void ezgrpc2_events_free(ezgrpc2_events *events); 

#endif
