#include <stdarg.h>
#include "defs.h"
#include "ezgrpc2_session.h"

//ezgrpc2_event *event_new(ezgrpc2_event_type type, ezgrpc2_session_uuid *session_uuid, ...) {
//  va_list ap;
//  va_start(ap, session_uuid);
//  ezgrpc2_event *event = malloc(sizeof(*event));
//  event->type = type;
//  event->session_uuid = session_uuid;
//  switch (type) {
//  case EZGRPC2_EVENT_MESSAGE:
//    event->message = va_arg(ap, ezgrpc2_event_message);
//    break;
//  case EZGRPC2_EVENT_CANCEL:
//    event->cancel = va_arg(ap, ezgrpc2_event_cancel);
//    break;
//  case EZGRPC2_EVENT_DATALOSS:
//    event->dataloss = va_arg(ap, ezgrpc2_event_dataloss);
//    break;
//  default:
//    break;
//  }
//  va_end(ap);
//
//  return event;
//}

