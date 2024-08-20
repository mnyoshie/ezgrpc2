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

enum ezgrpc2_status_code_t {
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

typedef enum ezgrpc2_status_code_t ezgrpc2_status_code_t;

enum ezgrpc2_event_type_t {
  /* The client has sent a message/s */
  EZGRPC2_EVENT_MESSAGE,
  /* The client has submitted an RST frame */
  EZGRPC2_EVENT_CANCEL,
  /* The client has sent a malformed grpc message with
   * an end stream
   */
  EZGRPC2_EVENT_DATALOSS,
};
typedef enum ezgrpc2_event_type_t ezgrpc2_event_type_t;



typedef struct ezgrpc2_server_t ezgrpc2_server_t;
struct ezgrpc2_server_t;
typedef struct ezgrpc2_session_t ezgrpc2_session_t;
struct ezgrpc2_session_t;
typedef struct ezgrpc2_stream_t ezgrpc2_stream_t;
struct ezgrpc2_stream_t;

typedef struct ezgrpc2_header_t ezgrpc2_header_t;
struct ezgrpc2_header_t {
  char *name, *value;
};

typedef struct ezgrpc2_event_cancel_t ezgrpc2_event_cancel_t;
struct ezgrpc2_event_cancel_t {
  i32 stream_id;
};

typedef struct ezgrpc2_event_message_t ezgrpc2_event_message_t;
struct ezgrpc2_event_message_t {
  char end_stream;
  i32 stream_id;
  /* cast list_popb to ``ezgrpc2_message_t *`` */
  list_t list_messages;
};

typedef struct ezgrpc2_event_dataloss_t ezgrpc2_event_dataloss_t;
struct ezgrpc2_event_dataloss_t {
  /* cast list_popb to ``ezgrpc2_message_t *`` */
  //list_t list_messages;
  i32 stream_id;
};

typedef struct ezgrpc2_event_t ezgrpc2_event_t;
struct ezgrpc2_event_t {

  //uint8_t session_id[32]; 
  char session_uuid[EZGRPC2_SESSION_UUID_LEN];
 // ezgrpc2_session_t *ezsession;
  ezgrpc2_event_type_t type;

  union {
    ezgrpc2_event_message_t message;
    ezgrpc2_event_dataloss_t dataloss;
    ezgrpc2_event_cancel_t cancel;
  };
};

typedef struct ezgrpc2_path_t ezgrpc2_path_t;
struct ezgrpc2_path_t {
  char *path;
  void *userdata;

  /* cast list_popb to ``ezgrpc2_event_t *`` */
  /* This contains events for this specific path */
  list_t list_events;
};


typedef struct ezvec_t ezvec_t;
struct ezvec_t {
  size_t len;
  u8 *data;
};


typedef struct ezgrpc2_message_t ezgrpc2_message_t;
struct ezgrpc2_message_t {
  u8 is_compressed;
  u32 len;
  i8 *data;
};




#ifdef __cplusplus
extern "C" {
#endif



ezgrpc2_server_t *ezgrpc2_server_init(
  const char *ipv4_addr, u16 ipv4_port,
  const char *ipv6_addr, u16 ipv6_port,
  int backlog);


/* the ezgrpc2_server_poll function polls the server for any clients making a request to paths
 * appointed by ``paths``.
 *
 * If requested path by the client is not found, a trailer, "EZGRPC2_STATUS_UNIMPLEMENTED"
 * is automatically sent. No vent is generated.
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

/* On success, ezgrpc2_session_send takes ownership of list_messages 
 *
 * A sucessful return value may mean:
 *
 *   1 The message was sent.
 *   2 The message is queued and pending, possible cause is
 *     when the client HTTP2 window is full.
 *
 *
 * Returns:
 *   
 *   0 on success
 *
 *   1 If the session doesn't exists
 *
 *   2 If the stream_id doesn't exists
 *
 */
int ezgrpc2_session_send(
  ezgrpc2_server_t *ezserver,
  char session_uuid[EZGRPC2_SESSION_UUID_LEN],
  i32 stream_id,
  list_t list_messages);

int ezgrpc2_session_end_stream(
  ezgrpc2_server_t *ezserver,
  char session_uuid[EZGRPC2_SESSION_UUID_LEN],
  i32 stream_id,
  ezgrpc2_status_code_t status);

int ezgrpc2_session_end_session(
  ezgrpc2_server_t *ezserver,
  char session_id[EZGRPC2_SESSION_UUID_LEN],
  i32 last_stream_id,
  ezgrpc2_status_code_t status);

/* list of ezgrpc2_header_t */
list_t ezgrpc2_session_get_headers(
  ezgrpc2_server_t *ezserver,
  char session_id[EZGRPC2_SESSION_UUID_LEN],
  i32 stream_id);

/* returns the value for the header `name`. */
/* returns NULL if it doesn't exists
 * (or memory allocation failed (unlikely)).
 *
 * The address returned is a nul terminated string
 * pointing to a dynamically allocated memory and must
 * be freed after use.
 * */
char *ezgrpc2_session_find_header(
  ezgrpc2_server_t *ezserver,
  char session_id[EZGRPC2_SESSION_UUID_LEN],
  char *name);

#ifdef __cplusplus
}
#endif

#endif /* EZGRPC2_H */
