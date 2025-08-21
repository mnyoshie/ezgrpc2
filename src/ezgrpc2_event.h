#ifndef EZGRPC2_EVENT_H
#define EZGRPC2_EVENT_H

#include "ezgrpc2_session_uuid.h"

#ifndef EZGRPC2_H
#error "ezgrpc2_event.h must not be included independently. include instead ezgrpc2.h"
#endif
typedef struct ezgrpc2_event_cancel_t ezgrpc2_event_cancel_t;
typedef struct ezgrpc2_event_message_t ezgrpc2_event_message_t;
typedef struct ezgrpc2_event_dataloss_t ezgrpc2_event_dataloss_t;
typedef struct ezgrpc2_event_t ezgrpc2_event_t;

typedef enum ezgrpc2_event_type_t ezgrpc2_event_type_t;
/**
 * Types of events 
 */
enum ezgrpc2_event_type_t {
  EZGRPC2_EVENT_MESSAGE,
  EZGRPC2_EVENT_CANCEL,
  EZGRPC2_EVENT_DATALOSS
};

/**
 *
 */
struct ezgrpc2_event_cancel_t {
  i32 stream_id;
};

/**
 *
 */
struct ezgrpc2_event_message_t {
  i32 stream_id;
  char end_stream;
  /* Cast the return of :c:func:`ezgrpc2_list_popb` to a pointer to
   * :c:struct:`ezgrpc2_message_t`
   * */
  ezgrpc2_list_t *lmessages;
};

/**
 *
 */
struct ezgrpc2_event_dataloss_t {
  /* cast ezgrpc2_list_popb to ``ezgrpc2_message_t *`` */
  i32 stream_id;
};

/**
 * This is the events stored in :c:member:`ezgrpc2_path_t.levents`.
 */
struct ezgrpc2_event_t {

  ezgrpc2_session_uuid_t *session_uuid;

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

#define ezgrpc2_event_new(a, b, c) ezgrpc2_event_new(a, b, c)
ezgrpc2_event_t *ezgrpc2_event_new(ezgrpc2_event_type_t type, ezgrpc2_session_uuid_t *session_uuid, ...);
void ezgrpc2_event_free(ezgrpc2_event_t *event);

#endif
