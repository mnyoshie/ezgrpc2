/* ezgrpc2.c - A (crude) gRPC server in C. */

#ifndef EZGRPC2_H
#define EZGRPC2_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>
#include "list.h"

#ifdef _WIN32
#  include <winsock2.h>
#  include <windows.h>
#endif

#define EZGRPC_MAX_SESSIONS 32
#define EZGRPC2_SESSION_UUID_LEN 37


typedef char i8;
typedef uint8_t u8;
typedef int8_t s8;

typedef int16_t i16;
typedef uint16_t u16;

typedef int32_t i32;
typedef uint32_t u32;

typedef int64_t i64;
typedef uint64_t u64;

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


typedef enum ezgrpc2_event_type_t ezgrpc2_event_type_t;
/**
 * Types of events 
 */
enum ezgrpc2_event_type_t {
  EZGRPC2_EVENT_MESSAGE,
  EZGRPC2_EVENT_CANCEL,
  EZGRPC2_EVENT_DATALOSS
};


typedef struct ezgrpc2_session_t ezgrpc2_session_t;
typedef struct ezgrpc2_server_t ezgrpc2_server_t;
/**
 * An opaque server context struct type returned by :c:func:`ezgrpc2_server_init()`.
 */
struct ezgrpc2_server_t;

typedef struct ezgrpc2_header_t ezgrpc2_header_t;
/**
 *
 */
struct ezgrpc2_header_t {
  size_t nlen;
  char *name;
  char *value;
  size_t vlen;
};

typedef struct ezgrpc2_event_cancel_t ezgrpc2_event_cancel_t;
/**
 *
 */
struct ezgrpc2_event_cancel_t {
  i32 stream_id;
};

typedef struct ezgrpc2_event_message_t ezgrpc2_event_message_t;
/**
 *
 */
struct ezgrpc2_event_message_t {
  char end_stream;
  i32 stream_id;
  /* Cast the return of :c:func:`list_popb` to a pointer to
   * :c:struct:`ezgrpc2_message_t`
   * */
  list_t list_messages;
};

typedef struct ezgrpc2_event_dataloss_t ezgrpc2_event_dataloss_t;
/**
 *
 */
struct ezgrpc2_event_dataloss_t {
  /* cast list_popb to ``ezgrpc2_message_t *`` */
  i32 stream_id;
};

typedef struct ezgrpc2_event_t ezgrpc2_event_t;
/**
 * This is the events stored in :c:member:`ezgrpc2_path_t.list_events`.
 */
struct ezgrpc2_event_t {

  char session_uuid[EZGRPC2_SESSION_UUID_LEN];
  ezgrpc2_event_type_t type;

  /**
   * Anonymous union
   */
  union {
    /**
     * When :c:member:`ezgrpc2_event_t.type` is :c:enumerator:`EZGRPC2_EVENT_MESSAGE`.
     */
    ezgrpc2_event_message_t message;
    /**
     * When :c:member:`ezgrpc2_event_t.type` is :c:enumerator:`EZGRPC2_EVENT_DATALOSS`.
     */
    ezgrpc2_event_dataloss_t dataloss;
    /**
     * When :c:member:`ezgrpc2_event_t.type` is :c:enumerator:`EZGRPC2_EVENT_CANCEL`.
     */
    ezgrpc2_event_cancel_t cancel;
  };
};

/**
 * A path to poll. To be passed to :c:func:`ezgrpc2_server_poll()`.
 */
typedef struct ezgrpc2_path_t ezgrpc2_path_t;
struct ezgrpc2_path_t {
  /**
   * Path to listen to. Must not contain an anchor or query
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
   * Cast the return of :c:func:`list_popb()` to a pointer to :c:struct:`ezgrpc2_event_t` when its argument id ``list_events``
   */
  list_t list_events;
};


typedef struct ezgrpc2_message_t ezgrpc2_message_t;
/**
 * A gRPC message
 */
struct ezgrpc2_message_t {
  u8 is_compressed;
  u32 len;
  i8 *data;
};



#ifdef __cplusplus
extern "C" {
#endif


/**
 * The :c:func:`ezgrpc2_server_init()` function creates a ezserver context,
 * binding to the associated ``ipv4_addr`` and ``ipv6_addr`` if it's not
 * `NULL`.
 *
 * At no point ``ipv4_addr`` and ``ipv6_addr`` be ``NULL.``
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
 *    ezgrpc2_server_t *ezgrpc2_server_init("0.0.0.0", 8080, "::", 8080, 16);
 *
 */
ezgrpc2_server_t *ezgrpc2_server_init(
  const char *ipv4_addr, u16 ipv4_port,
  const char *ipv6_addr, u16 ipv6_port,
  int backlog);


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
 *    The :c:member:`ezgrpc2_path_t.list_events` needs to be
 *    initialized first with, :c:func:`list_init()`.
 *
 *
 */
int ezgrpc2_server_poll(
  ezgrpc2_server_t *server,
  ezgrpc2_path_t *paths,
  size_t nb_paths,
  int timeout);

void ezgrpc2_server_destroy(
  ezgrpc2_server_t *server);

//int ezgrpc2_session_submit_response(ezgrpc2_session_t *ezsession, i32 stream_id, list_t *list_messages, int end_stream, int grpc_status);

/**
 * The :c:func:`ezgrpc2_session_send()` sends the  messages in the list,
 * ``list_messages`` to the associated session_uuid and stream_id.

 * On success, the :c:func:`ezgrpc2_session_send()` takes ownership of list_messages 
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
  char session_uuid[EZGRPC2_SESSION_UUID_LEN],
  i32 stream_id,
  list_t list_messages);

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
  char session_uuid[EZGRPC2_SESSION_UUID_LEN],
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
  char session_id[EZGRPC2_SESSION_UUID_LEN],
  i32 last_stream_id,
  ezgrpc2_status_t status);

#if 0
/* list of ezgrpc2_header_t */
list_t ezgrpc2_session_get_headers(
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
  char session_uuid[EZGRPC2_SESSION_UUID_LEN],
  i32 stream_id,
  ezgrpc2_header_t *ezheader);

#ifdef __cplusplus
}
#endif

#endif /* EZGRPC2_H */
