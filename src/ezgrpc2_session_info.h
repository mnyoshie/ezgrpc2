#ifndef EZGRPC2_SESSION_INFO_H
#define EZGRPC2_SESSION_INFO_H

typedef struct ezgrpc2_session_info_t ezgrpc2_session_info_t;

struct ezgrpc2_session_info_t {
  ezgrpc2_session_uuid_t *session_uuid;
  size_t nb_open_streams;
  uint64_t time_since_last_request;
};

#endif
