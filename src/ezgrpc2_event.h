#ifndef EZGRPC2_EVENT_H
#define EZGRPC2_EVENT_H

#include "common.h"
#include "ezgrpc2_list.h"
#include "ezgrpc2_session_uuid.h"

typedef struct ezgrpc2_event_cancel_t ezgrpc2_event_cancel_t;
typedef struct ezgrpc2_event_message_t ezgrpc2_event_message_t;
typedef struct ezgrpc2_event_dataloss_t ezgrpc2_event_dataloss_t;
typedef struct ezgrpc2_event ezgrpc2_event;

/**
 * Types of events 
 */
enum ezgrpc2_event_type_t {
  EZGRPC2_EVENT_CONNECT,
  EZGRPC2_EVENT_DISCONNECT,
  EZGRPC2_EVENT_MESSAGE,
  EZGRPC2_EVENT_CANCEL,
  EZGRPC2_EVENT_DATALOSS
};
typedef enum ezgrpc2_event_type_t ezgrpc2_event_type_t;

/**
 *
 */
struct ezgrpc2_event_cancel_t {
  size_t path_index;
  i32 stream_id;
};

/**
 *
 */
struct ezgrpc2_event_message_t {
  size_t path_index;
  i32 stream_id;
  char end_stream;
  /* Cast the return of :c:func:`ezgrpc2_list_popb` to a pointer to
   * :c:struct:`ezgrpc2_message`
   * */
  ezgrpc2_list *lmessages;
};

/**
 *
 */
struct ezgrpc2_event_dataloss_t {
  size_t path_index;
  /* cast ezgrpc2_list_popb to ``ezgrpc2_message *`` */
  i32 stream_id;
};

/**
 * This is the events stored in :c:member:`ezgrpc2_path.levents`.
 */
struct ezgrpc2_event {

  ezgrpc2_session_uuid *session_uuid;

  ezgrpc2_event_type_t type;

  /**
   * Anonymous union
   */
  union {
    /**
     * When :c:member:`ezgrpc2_event.type` is :c:enumerator:`EZGRPC2_EVENT_MESSAGE`.
     */
    ezgrpc2_event_message_t message;
    /**
     * When :c:member:`ezgrpc2_event.type` is :c:enumerator:`EZGRPC2_EVENT_DATALOSS`.
     */
    ezgrpc2_event_dataloss_t dataloss;
    /**
     * When :c:member:`ezgrpc2_event.type` is :c:enumerator:`EZGRPC2_EVENT_CANCEL`.
     */
    ezgrpc2_event_cancel_t cancel;
  };
};

void ezgrpc2_event_free(ezgrpc2_event *event);

#endif
