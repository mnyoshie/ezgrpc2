#ifndef EZGRPC2_EVENT_H
#define EZGRPC2_EVENT_H

#include <stdint.h>
#include "defs.h"
#include "ezgrpc2_messages.h"
#include "ezgrpc2_session_uuid.h"

typedef struct ezgrpc2_event_cancel ezgrpc2_event_cancel;
typedef struct ezgrpc2_event_message ezgrpc2_event_message;
typedef struct ezgrpc2_event_dataloss ezgrpc2_event_dataloss;
typedef struct ezgrpc2_event ezgrpc2_event;

/**
 * Types of events 
 */
enum ezgrpc2_event_type {
  EZGRPC2_EVENT_CONNECT,
  EZGRPC2_EVENT_DISCONNECT,
  EZGRPC2_EVENT_MESSAGE,
  EZGRPC2_EVENT_CANCEL,
  EZGRPC2_EVENT_DATALOSS
};
typedef enum ezgrpc2_event_type ezgrpc2_event_type;

/**
 *
 */
struct ezgrpc2_event_cancel {
  int32_t stream_id;
};

/**
 *
 */
struct ezgrpc2_event_message {
  /* Cast the return of :c:func:`ezgrpc2_list_popb` to a pointer to
   * :c:struct:`ezgrpc2_message`
   * */
  ezgrpc2_messages *messages;
  int32_t stream_id;
  char end_stream;
};

/**
 *
 */
struct ezgrpc2_event_dataloss {
  /* cast ezgrpc2_list_popb to ``ezgrpc2_message *`` */
  int32_t stream_id;
};

/**
 * This is the events stored in :c:member:`ezgrpc2_path.levents`.
 */
struct ezgrpc2_event {
  ezgrpc2_session_uuid session_uuid;
  /**
   * Anonymous union
   */
  union {
    /**
     * When :c:member:`ezgrpc2_event.type` is :c:enumerator:`EZGRPC2_EVENT_MESSAGE`.
     */
    ezgrpc2_event_message message;
    /**
     * When :c:member:`ezgrpc2_event.type` is :c:enumerator:`EZGRPC2_EVENT_DATALOSS`.
     */
    ezgrpc2_event_dataloss dataloss;
    /**
     * When :c:member:`ezgrpc2_event.type` is :c:enumerator:`EZGRPC2_EVENT_CANCEL`.
     */
    ezgrpc2_event_cancel cancel;
  };
  void *userdata;


  ezgrpc2_event_type type;
};

//void ezgrpc2_event_free(ezgrpc2_event *event);

#endif
