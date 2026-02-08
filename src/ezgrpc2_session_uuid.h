#ifndef EZGRPC2_SESSION_UUID_H
#define EZGRPC2_SESSION_UUID_H
#include "defs.h"

#ifdef _WIN32
#include <Rpc.h>
#else
#include <uuid/uuid.h>
#endif

typedef struct ezgrpc2_session_uuid ezgrpc2_session_uuid;
struct ezgrpc2_session_uuid {
#ifdef _WIN32
  UUID uuid;
#else
  uuid_t uuid;
#endif
  size_t index;
};

EZGRPC2_API int ezgrpc2_session_uuid_is_equal(ezgrpc2_session_uuid uuid1, ezgrpc2_session_uuid uuid2);
//ezgrpc2_session_uuid *ezgrpc2_session_uuid_new(void *unused);
EZGRPC2_API ezgrpc2_session_uuid ezgrpc2_session_uuid_copy(ezgrpc2_session_uuid session_uuid);
#endif
