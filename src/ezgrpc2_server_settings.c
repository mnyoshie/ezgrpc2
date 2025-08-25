#include <stdlib.h>
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
  .write_cb = write_cb,
  .read_cb = read_cb,
  .initial_window_size = 1 << 20,
  .max_frame_size = 16*1024,
  .max_concurrent_streams = 32
};

ezgrpc2_server_settings_t *ezgrpc2_server_settings_new(void *unused) {
  (void)unused;
  ezgrpc2_server_settings_t *server_settings = malloc(sizeof(*server_settings));
  memcpy(server_settings, &default_server_settings, sizeof(ezgrpc2_server_settings_t));
  return server_settings;
}

void ezgrpc2_server_settings_set_initial_window_size(ezgrpc2_server_settings_t *server_settings, size_t initial_window_size) {
  server_settings->initial_window_size = initial_window_size;
}

void ezgrpc2_server_settings_set_max_frame_size(ezgrpc2_server_settings_t *server_settings, size_t max_frame_size) {
  server_settings->max_frame_size = max_frame_size;
}

void ezgrpc2_server_settings_set_max_concurrent_streams(ezgrpc2_server_settings_t *server_settings, size_t max_concurrent_streams) {
  server_settings->max_concurrent_streams = max_concurrent_streams;
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

void ezgrpc2_server_settings_free(ezgrpc2_server_settings_t *server_settings) {
  free(server_settings);
}
