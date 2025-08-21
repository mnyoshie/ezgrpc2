/* ezgrpc2.c - A (crude) gRPC server in C. */

#ifndef EZGRPC2_H
#define EZGRPC2_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>
#include "common.h"

#include "ezgrpc2_list.h"
#include "ezgrpc2_event.h"
#include "ezgrpc2_session_uuid.h"
#include "ezgrpc2_message.h"

#ifdef _WIN32
#  include <winsock2.h>
#  include <windows.h>
#endif

#define EZGRPC_MAX_SESSIONS 32
//#define EZGRPC2_SESSION_UUID_LEN 37



typedef enum ezgrpc2_status_t ezgrpc2_status_t;
/**
 * Types of trailer status codes
 */
enum ezgrpc2_status_t {
  /* https://github.com/grpc/grpc/tree/master/include/grpcpp/impl/codegen */
  EZGRPC2_STATUS_OK = 0,
  EZGRPC2_STATUS_CANCELLED = 1,
  EZGRPC2_STATUS_UNKNOWN = 2,
  EZGRPC2_STATUS_INVALID_ARGUMENT = 3,
  EZGRPC2_STATUS_DEADLINE_EXCEEDED = 4,
  EZGRPC2_STATUS_NOT_FOUND = 5,
  EZGRPC2_STATUS_ALREADY_EXISTS = 6,
  EZGRPC2_STATUS_PERMISSION_DENIED = 7,
  EZGRPC2_STATUS_RESOURCE_EXHAUSTED = 8,
  EZGRPC2_STATUS_FAILED_PRECONDITION = 9,
  EZGRPC2_STATUS_OUT_OF_RANGE = 11,
  EZGRPC2_STATUS_UNIMPLEMENTED = 12,
  EZGRPC2_STATUS_INTERNAL = 13,
  EZGRPC2_STATUS_UNAVAILABLE = 14,
  EZGRPC2_STATUS_DATA_LOSS = 15,
  EZGRPC2_STATUS_UNAUTHENTICATED = 16,
  EZGRPC2_STATUS_NULL = -1
};



typedef struct ezgrpc2_server_t ezgrpc2_server_t;
typedef struct ezgrpc2_header_t ezgrpc2_header_t;
typedef struct ezgrpc2_path_t ezgrpc2_path_t;

/**
 * An opaque server context struct type returned by :c:func:`ezgrpc2_server_new()`.
 */
struct ezgrpc2_server_t;


/**
 *
 */
struct ezgrpc2_header_t {
  size_t nlen;
  char *name;
  char *value;
  size_t vlen;
};


/**
 * A path to poll. To be passed to :c:func:`ezgrpc2_server_poll()`.
 */
struct ezgrpc2_path_t {
  /**
   * Path to listen to. Must not contain an anchor or a query (# or ?)
   */
  char *path;

  /**
   * User defined userdata
   */
  void *userdata;

  /**
   * This is a list of :c:struct:`ezgrpc2_event_t` and it
   * contains events for this specific path.
   *
   * Cast the return of :c:func:`ezgrpc2_list_pop_first()` to a pointer to :c:struct:`ezgrpc2_event_t` when its argument id ``levents``
   */
  ezgrpc2_list_t *levents;
};





#ifdef __cplusplus
extern "C" {
#endif


/**
 * The :c:func:`ezgrpc2_server_new()` function creates a ezserver context,
 * binding to the associated ``ipv4_addr`` and ``ipv6_addr`` if it's not
 * ``NULL``.
 *
 * At no point ``ipv4_addr`` and ``ipv6_addr`` be ``NULL.``
 *
 * :param settings: Unused. Reserved for future implementation. Must be set to ``NULL``.
 *
 * :returns:
 *
 *     * On success, a pointer to the opaque :c:struct:`ezgrpc2_server_t`
 *
 *     * On failure, ``NULL``.
 *
 * Example 1:
 *
 * .. code-block:: C
 *
 *    ezgrpc2_server_t *ezgrpc2_server_new("0.0.0.0", 8080, "::", 8080, 16, NULL);
 *
 */
ezgrpc2_server_t *ezgrpc2_server_new(
  const char *ipv4_addr, u16 ipv4_port,
  const char *ipv6_addr, u16 ipv6_port,
  int backlog,
  void *settings);


/**
 * The :c:func:`ezgrpc2_server_poll()` function polls the server for any clients making a request to paths
 * appointed by ``paths``.
 *
 * If the requested path by the client is not found, a trailer, :c:enumerator:`EZGRPC2_STATUS_UNIMPLEMENTED`
 * is automatically sent and closes the associated stream. No event is generated.
 *
 * :returns:
 *
 *    * On an event, a value greater than 0.
 *
 *    * On no event, 0.
 *
 *    * On error, a negative value.
 *
 * .. note::
 *    The :c:member:`ezgrpc2_path_t.levents` needs to be
 *    initialized first with, :c:func:`ezgrpc2_list_init()`.
 *
 *
 */
int ezgrpc2_server_poll(
  ezgrpc2_server_t *server,
  ezgrpc2_path_t *paths,
  size_t nb_paths,
  int timeout);

void ezgrpc2_server_free(
  ezgrpc2_server_t *server);

//int ezgrpc2_session_submit_response(ezgrpc2_session_t *ezsession, i32 stream_id, ezgrpc2_list_t *lmessages, int end_stream, int grpc_status);

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
int ezgrpc2_session_send(
  ezgrpc2_server_t *ezserver,
  ezgrpc2_session_uuid_t *session_uuid,
  i32 stream_id,
  ezgrpc2_list_t *lmessages);

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
int ezgrpc2_session_end_stream(
  ezgrpc2_server_t *ezserver,
  ezgrpc2_session_uuid_t *session_uuid,
  i32 stream_id,
  ezgrpc2_status_t status);

/**
 * The :c:func:`ezgrpc2_session_end_session()` ends the session
 * associated with the session_uuid.
 *
 * :returns:
 *    * On success, 0.
 *
 *    * If the session doesn't exists, 1.
 */
int ezgrpc2_session_end_session(
  ezgrpc2_server_t *ezserver,
  ezgrpc2_session_uuid_t *session_id,
  i32 last_stream_id,
  ezgrpc2_status_t status);

#if 0
/* list of ezgrpc2_header_t */
ezgrpc2_list_t ezgrpc2_session_get_headers(
  ezgrpc2_server_t *ezserver,
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
 *    ezgrpc2_header_t ezheader = {.name = "content-type", .nlen = 12};
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
int ezgrpc2_session_find_header(
  ezgrpc2_server_t *ezserver,
  ezgrpc2_session_uuid_t *session_uuid,
  i32 stream_id,
  ezgrpc2_header_t *ezheader);


#ifdef __cplusplus
}
#endif

#endif /* EZGRPC2_H */
