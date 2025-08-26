#include "ezgrpc2_global.h"
#ifdef _WIN32
#include <Winsock2.h>
#endif

int ezgrpc2_global_init(uint64_t unused) {
  (void)unused;
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
void ezgrpc2_global_cleanup() {
#ifdef _WIN32
  WSACleanup();
#endif
}
