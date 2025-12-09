#ifndef EZGRPC2_SESSION_UUID_STRUCT_H
#define EZGRPC2_SESSION_UUID_STRUCT_H
#ifdef _WIN32
#include <Rpc.h>
#else
#include <uuid/uuid.h>
#endif

struct ezgrpc2_session_uuid {
  size_t index;
#ifdef _WIN32
  UUID uuid;
#else
  uuid_t uuid;
#endif
};
#endif
