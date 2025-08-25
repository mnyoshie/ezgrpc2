#include <stdlib.h>

#ifdef _WIN32
#include <Rpc.h>
#else
#include <uuid/uuid.h>
#endif

#include "ezgrpc2_session_uuid.h"

int ezgrpc2_session_uuid_is_equal(ezgrpc2_session_uuid_t *uuid1, ezgrpc2_session_uuid_t *uuid2) {
  if (uuid1 == NULL || uuid2 == NULL)
    return 0;
#ifdef _WIN32
  return !memcmp(uuid1, uuid2, sizeof(UUID));
#else
  return !memcmp(uuid1, uuid2, UUID_STR_LEN);
#endif
}

ezgrpc2_session_uuid_t *ezgrpc2_session_uuid_new(void *unused) {
  (void)unused;
#ifdef _WIN32
  ezgrpc2_session_uuid_t *session_uuid = malloc(sizeof(UUID));
  RPC_STATUS res = UuidCreate((UUID*)session_uuid);
  assert(res == RPC_S_OK);
#else
  ezgrpc2_session_uuid_t *session_uuid = malloc(UUID_STR_LEN);
  uuid_t uuid;
  uuid_generate_random(uuid);
  uuid_unparse_lower(uuid, session_uuid);
#endif
  return session_uuid;
}

ezgrpc2_session_uuid_t *ezgrpc2_session_uuid_copy(ezgrpc2_session_uuid_t *session_uuid) {
#ifdef _WIN32
  ezgrpc2_session_uuid_t *ret = malloc(sizeof(UUID));
  memcpy(ret, session_uuid, sizeof(UUID));
#else
  ezgrpc2_session_uuid_t *ret = malloc(UUID_STR_LEN);
  memcpy(ret, session_uuid, UUID_STR_LEN);
#endif
  return ret;
}
void ezgrpc2_session_uuid_free(ezgrpc2_session_uuid_t *session_uuid) {
  free(session_uuid);
}
