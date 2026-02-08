#ifndef EZGRPC2_SESSION_INFO_H
#define EZGRPC2_SESSION_INFO_H

#include "ezgrpc2_session_uuid.h"

typedef struct ezgrpc2_session_info ezgrpc2_session_info;

struct ezgrpc2_session_info {
  ezgrpc2_session_uuid *session_uuid;
  size_t nb_open_streams;
  uint64_t time_since_last_request;
};

#endif
