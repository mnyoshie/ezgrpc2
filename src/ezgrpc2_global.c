#include <stdio.h>
#include "core.h"
#include "ezgrpc2_global.h"
#ifdef _WIN32
#include <Winsock2.h>
#endif

#include "ezgrpc2_events_struct.h"

EZGRPC2_API int ezgrpc2_global_init(uint64_t unused) {
  (void)unused;
#ifdef EZGRPC2_DEBUG
#define PRINT_SIZE(x) printf(#x ": %zu\n", sizeof(x))
  PRINT_SIZE(ezgrpc2_server);
  PRINT_SIZE(ezgrpc2_session);
  PRINT_SIZE(ezgrpc2_event);
  PRINT_SIZE(ezgrpc2_events);
  PRINT_SIZE(ezgrpc2_message);
#endif

#ifdef _WIN32
  int ret;
  WSADATA wsa_data = {0};
  if ((ret = WSAStartup(0x0202, &wsa_data)) != 0) {
    fprintf(stderr, "WSAStartup failed: error %d\n", ret);
    return 1;
  }
#endif
  return 0;
}
EZGRPC2_API void ezgrpc2_global_cleanup() {
#ifdef _WIN32
  WSACleanup();
#endif
}
