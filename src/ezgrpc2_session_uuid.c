#include <stdlib.h>
#include <assert.h>
#include "core.h"

#ifdef _WIN32
#include <Rpc.h>
#else
#include <uuid/uuid.h>
#endif

#include "ezgrpc2_session_uuid.h"

EZGRPC2_API int ezgrpc2_session_uuid_is_equal(ezgrpc2_session_uuid *uuid1, ezgrpc2_session_uuid *uuid2) {
  if (uuid1 == NULL || uuid2 == NULL)
    return 0;
#ifdef _WIN32
  return !memcmp(uuid1, uuid2, sizeof(UUID));
#else
  return !memcmp(uuid1, uuid2, sizeof(uuid_t));
#endif
}

//ezgrpc2_session_uuid *ezgrpc2_session_uuid_new(void *unused) {
//  (void)unused;
//#ifdef _WIN32
//  ezgrpc2_session_uuid *session_uuid = malloc(sizeof(UUID));
//  RPC_STATUS res = UuidCreate((UUID*)session_uuid);
//  assert(res == RPC_S_OK);
//#else
//  ezgrpc2_session_uuid *session_uuid = malloc(UUID_STR_LEN);
//  uuid_t uuid;
//  uuid_generate_random(uuid);
//  uuid_unparse_lower(uuid, session_uuid);
//#endif
//  return session_uuid;
//}

EZGRPC2_API ezgrpc2_session_uuid *ezgrpc2_session_uuid_copy(ezgrpc2_session_uuid *session_uuid) {
#ifdef _WIN32
  ezgrpc2_session_uuid *ret = malloc(sizeof(UUID));
  memcpy(ret, session_uuid, sizeof(UUID));
#else
  ezgrpc2_session_uuid *ret = malloc(sizeof(uuid_t));
  memcpy(ret, session_uuid, sizeof(uuid_t));
#endif
  return ret;
}

EZGRPC2_API void ezgrpc2_session_uuid_free(ezgrpc2_session_uuid *session_uuid) {
  free(session_uuid);
}
