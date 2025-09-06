#include <stdarg.h>
#include "core.h"
#include "ezgrpc2.h"

ezgrpc2_event_t *event_new(ezgrpc2_event_type_t type, ezgrpc2_session_uuid_t *session_uuid, ...) {
  va_list ap;
  va_start(ap, session_uuid);
  ezgrpc2_event_t *event = malloc(sizeof(*event));
  event->type = type;
  event->session_uuid = session_uuid;
  switch (type) {
  case EZGRPC2_EVENT_MESSAGE:
    event->message = va_arg(ap, ezgrpc2_event_message_t);
    break;
  case EZGRPC2_EVENT_CANCEL:
    event->cancel = va_arg(ap, ezgrpc2_event_cancel_t);
    break;
  case EZGRPC2_EVENT_DATALOSS:
    event->dataloss = va_arg(ap, ezgrpc2_event_dataloss_t);
    break;
  default:
    break;
  }

  return event;
}

EZGRPC2_API void ezgrpc2_event_free(ezgrpc2_event_t *event) {
  if (event == NULL) return;

  ezgrpc2_session_uuid_free(event->session_uuid);
  switch (event->type) {
  case EZGRPC2_EVENT_MESSAGE: {
    ezgrpc2_message_t *message;
    if (event->message.lmessages != NULL)
      while ((message = ezgrpc2_list_pop_front(event->message.lmessages)) !=
             NULL) {
        free(message->data);
        free(message);
      }
    ezgrpc2_list_free(event->message.lmessages);
  } break;
  case EZGRPC2_EVENT_CANCEL:
  case EZGRPC2_EVENT_DATALOSS:
    break;
  default:
    break;
  }

  free(event);
}
