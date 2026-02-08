#include "core.h"
#include "ezgrpc2_session_info.h"

ezgrpc2_session_info *session_info_new(void *unused) {
  (void)unused;
  ezgrpc2_session_info *session_info = malloc(sizeof(*session_info));
  return session_info;
}

