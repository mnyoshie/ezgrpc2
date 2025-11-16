#ifndef EZGRPC2_SESSION_UUID_H
#define EZGRPC2_SESSION_UUID_H
typedef void ezgrpc2_session_uuid;
int ezgrpc2_session_uuid_is_equal(ezgrpc2_session_uuid *uuid1, ezgrpc2_session_uuid *uuid2);
//ezgrpc2_session_uuid *ezgrpc2_session_uuid_new(void *unused);
ezgrpc2_session_uuid *ezgrpc2_session_uuid_copy(ezgrpc2_session_uuid *session_uuid);
void ezgrpc2_session_uuid_free(ezgrpc2_session_uuid *uuid);
#endif
