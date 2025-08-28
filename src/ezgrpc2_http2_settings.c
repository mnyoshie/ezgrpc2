#include <stdlib.h>
#include "ezgrpc2_http2_settings.h"
#include "ezgrpc2_http2_settings_struct.h"

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

static ezgrpc2_http2_settings_t default_http2_settings = {
  .initial_window_size = 1 << 20,
  .max_frame_size = 16*1024,
  .max_concurrent_streams = 32
};

ezgrpc2_http2_settings_t *ezgrpc2_http2_settings_new(void *unused) {
  (void)unused;
  ezgrpc2_http2_settings_t *http2_settings = malloc(sizeof(*http2_settings));
  memcpy(http2_settings, &default_http2_settings, sizeof(ezgrpc2_http2_settings_t));
  return http2_settings;
}

void ezgrpc2_http2_settings_set_initial_window_size(ezgrpc2_http2_settings_t *http2_settings, size_t initial_window_size) {
  http2_settings->initial_window_size = initial_window_size;
}

void ezgrpc2_http2_settings_set_max_frame_size(ezgrpc2_http2_settings_t *http2_settings, size_t max_frame_size) {
  http2_settings->max_frame_size = max_frame_size;
}

void ezgrpc2_http2_settings_set_max_concurrent_streams(ezgrpc2_http2_settings_t *http2_settings, size_t max_concurrent_streams) {
  http2_settings->max_concurrent_streams = max_concurrent_streams;
}
//void ezgrpc2_http2_settings_set_write_callback(ezgrpc2_http2_settings_t *http2_settings,
//    ssize_t (*write)(void *sock, void *buf, size_t len, int flags)
//    ){
//  http2_settings->write_cb = write;
//}
//
//void ezgrpc2_http2_settings_set_read_callback(ezgrpc2_http2_settings_t *http2_settings,
//    ssize_t (*read)(void *sock, void *buf, size_t len, int flags)
//    ){
//  http2_settings->read_cb = read;
//}

void ezgrpc2_http2_settings_free(ezgrpc2_http2_settings_t *http2_settings) {
  free(http2_settings);
}
