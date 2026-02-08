#ifndef EZGRPC2_EVENTS_STRUCT_H
#define EZGRPC2_EVENTS_STRUCT_H

#include "ezgrpc2_events.h"
#include "ezgrpc2_list_struct.h"


struct ezgrpc2_events {
  /* list of array ezgrpc2_event[X] */
  ezgrpc2_list bevents;
  /* 0 to X-1 */
  //size_t rindex
  size_t windex;
};

int ezgrpc2_events_write(ezgrpc2_events *events, ezgrpc2_event event);
#endif
