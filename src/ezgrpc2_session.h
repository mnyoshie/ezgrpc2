/* ezgrpc2.c - A (crude) gRPC server in C. */

#ifndef EZGRPC2_SESSION_H
#define EZGRPC2_SESSION_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>
#include "common.h"

#include "ezgrpc2_event.h"
#include "ezgrpc2_global.h"
#include "ezgrpc2_http2_settings.h"
#include "ezgrpc2_list_event.h"
#include "ezgrpc2_list_message.h"
#include "ezgrpc2_server.h"
#include "ezgrpc2_server_settings.h"
#include "ezgrpc2_session_uuid.h"
#include "ezgrpc2_message.h"
#include "ezgrpc2_path.h"
#include "ezgrpc2_header.h"
#include "ezgrpc2_grpc_status.h"



#ifdef __cplusplus
extern "C" {
#endif

//int ezgrpc2_session_submit_response(ezgrpc2_session *ezsession, i32 stream_id, ezgrpc2_list *lmessages, int end_stream, int grpc_status);

/**
 * The :c:func:`ezgrpc2_session_send()` sends the  messages in the list,
 * ``lmessages`` to the associated session_uuid and stream_id.

 * On success, the :c:func:`ezgrpc2_session_send()` takes ownership of lmessages 
 *
 * A sucessful return value may mean:
 *
 * * The message was sent.
 *
 * * The message is queued and pending, possible cause is
 *   when the client HTTP2 window is full or the socket which have been
 *   marked as non-blocking returns ``EWOULDBLOCK``.
 *
 * :returns:
 *    * On success, 0.
 *
 *    * If the session doesn't exists, 1.
 *
 *    * If the stream_id doesn't exists, 2.
 *
 */
int ezgrpc2_server_session_stream_send(
  ezgrpc2_server *ezserver,
  ezgrpc2_session_uuid *session_uuid,
  i32 stream_id,
  ezgrpc2_list *lmessages);

/**
 * The :c:func:`ezgrpc2_session_end_stream()` ends the stream associated with the session_uuid and stream_id.
 *
 * :returns:
 *    * On success, 0.
 *
 *    * If the session doesn't exists, 1.
 *
 *    * If the stream_id doesn't exists, 2.
 */
int ezgrpc2_server_session_stream_end(
  ezgrpc2_server *ezserver,
  ezgrpc2_session_uuid *session_uuid,
  i32 stream_id,
  ezgrpc2_grpc_status status);

/**
 * The :c:func:`ezgrpc2_server_session_end()` ends the session
 * associated with the session_uuid.
 *
 * :returns:
 *    * On success, 0.
 *
 *    * If the session doesn't exists, 1.
 */
int ezgrpc2_server_session_end(
  ezgrpc2_server *ezserver,
  ezgrpc2_session_uuid *session_id,
  i32 last_stream_id,
  int status);

#if 0
/* list of ezgrpc2_header */
ezgrpc2_list ezgrpc2_session_get_headers(
  ezgrpc2_server *ezserver,
  char session_id[EZGRPC2_SESSION_UUID_LEN],
  i32 stream_id);
#endif

/**
 * The :c:func:`ezgrpc2_session_find_header()` finds the associated header value
 * for the matching header `name`.
 *
 * Example 1:
 *
 * .. code-block:: C
 *
 *    ezgrpc2_header ezheader = {.name = "content-type", .nlen = 12};
 *    if (!ezgrpc2_session_find_header(ezserver, session_uuid, stream_id, &ezheader))
 *      printf("Found value %.*s\n", ezheader.vlen, ezheader.value);
 *
 * If name is found, ezheader.value and ezheader.vlen is initialized.
 *
 * The address in ezheader.value is owned by the library. It must not
 * be freed or modified. User should make a copy of this value if required.
 *
 * :returns:
 *    * If header name is found, 0
 *
 *    * If the session doesn't exists, 1
 *
 *    * If the stream_id doesn't exists, 2
 *
 *    * If header name is not found, 3
 *
 * .. note::
 *    Strings are compared ignoring case
 * */
int ezgrpc2_server_session_stream_find_header(
  ezgrpc2_server *ezserver,
  ezgrpc2_session_uuid *session_uuid,
  i32 stream_id,
  ezgrpc2_header *ezheader);

const char *ezgrpc2_license(void);

#ifdef __cplusplus
}
#endif

#endif /* EZGRPC2_H */
