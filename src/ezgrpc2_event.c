#include <stdarg.h>
#include "core.h"
#include "ezgrpc2_session.h"

ezgrpc2_event *event_new(ezgrpc2_event_type type, ezgrpc2_session_uuid *session_uuid, ...) {
  va_list ap;
  va_start(ap, session_uuid);
  ezgrpc2_event *event = malloc(sizeof(*event));
  event->type = type;
  event->session_uuid = session_uuid;
  switch (type) {
  case EZGRPC2_EVENT_MESSAGE:
    event->message = va_arg(ap, ezgrpc2_event_message);
    break;
  case EZGRPC2_EVENT_CANCEL:
    event->cancel = va_arg(ap, ezgrpc2_event_cancel);
    break;
  case EZGRPC2_EVENT_DATALOSS:
    event->dataloss = va_arg(ap, ezgrpc2_event_dataloss);
    break;
  default:
    break;
  }

  return event;
}

EZGRPC2_API void ezgrpc2_event_free(ezgrpc2_event *event) {
  if (event == NULL) return;

  switch (event->type) {
  case EZGRPC2_EVENT_MESSAGE: {
    ezgrpc2_message *message;
    if (event->message.lmessages != NULL)
      while ((message = ezgrpc2_list_pop_front(event->message.lmessages)) !=
             NULL) {
        ezgrpc2_message_free(message);
      }
    ezgrpc2_list_free(event->message.lmessages);
  } break;
  case EZGRPC2_EVENT_DISCONNECT:
    ezgrpc2_session_uuid_free(event->session_uuid);
  case EZGRPC2_EVENT_CONNECT:
  case EZGRPC2_EVENT_CANCEL:
  case EZGRPC2_EVENT_DATALOSS:
    break;
  default:
    break;
  }

  free(event);
}
