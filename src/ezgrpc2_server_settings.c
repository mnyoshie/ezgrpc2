#include <stdlib.h>
#include "core.h"
#include "ezgrpc2_server_settings.h"
#include "ezgrpc2_server_settings_struct.h"

#ifdef _WIN32
#  include <ws2tcpip.h>
#  include <winsock2.h>
#  include <windows.h>
#else
#include <sys/socket.h>
#endif

static ssize_t write_cb(void *const sock, void *buf, size_t len, int flags) {
#ifdef _WIN32
  return send(*(SOCKET*)sock, buf, len, flags);
#else
  return send(*(int* const)sock, buf, len, flags);
#endif
}

static ssize_t read_cb(void *const sock, void *buf, size_t len, int flags) {
#ifdef _WIN32
  return recv(*(SOCKET* const)sock, buf, len, flags);
#else
  return recv(*(int* const)sock, buf, len, flags);
#endif
}

static ezgrpc2_server_settings_t default_server_settings = {
  .max_connections = 1024,
  .logging_level = 0,
};

EZGRPC2_API ezgrpc2_server_settings_t *ezgrpc2_server_settings_new(void *unused) {
  (void)unused;
  ezgrpc2_server_settings_t *server_settings = malloc(sizeof(*server_settings));
  memcpy(server_settings, &default_server_settings, sizeof(ezgrpc2_server_settings_t));
  server_settings->logging_fp = stderr;
  return server_settings;
}

EZGRPC2_API void ezgrpc2_server_settings_set_max_connections(ezgrpc2_server_settings_t *server_settings, size_t max_connections) {
  server_settings->max_connections = max_connections;
}

EZGRPC2_API void ezgrpc2_server_settings_set_logging_fp(ezgrpc2_server_settings_t *server_settings, FILE *fp) {
  server_settings->logging_fp = fp;
}

EZGRPC2_API void ezgrpc2_server_settings_set_logging_level(ezgrpc2_server_settings_t *server_settings, uint32_t logging_level) {
  server_settings->logging_level = logging_level;
}
//void ezgrpc2_server_settings_set_write_callback(ezgrpc2_server_settings_t *server_settings,
//    ssize_t (*write)(void *sock, void *buf, size_t len, int flags)
//    ){
//  server_settings->write_cb = write;
//}
//
//void ezgrpc2_server_settings_set_read_callback(ezgrpc2_server_settings_t *server_settings,
//    ssize_t (*read)(void *sock, void *buf, size_t len, int flags)
//    ){
//  server_settings->read_cb = read;
//}

EZGRPC2_API void ezgrpc2_server_settings_free(ezgrpc2_server_settings_t *server_settings) {
  free(server_settings);
}
