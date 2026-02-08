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

static ezgrpc2_http2_settings default_http2_settings = {
  .initial_window_size = 1 << 20,
  .max_frame_size = 16*1024,
  .max_concurrent_streams = 1024,
  .max_paths = 10
};

ezgrpc2_http2_settings *ezgrpc2_http2_settings_new(void *unused) {
  (void)unused;
  ezgrpc2_http2_settings *http2_settings = malloc(sizeof(*http2_settings));
  memcpy(http2_settings, &default_http2_settings, sizeof(ezgrpc2_http2_settings));
  return http2_settings;
}

void ezgrpc2_http2_settings_set_initial_window_size(ezgrpc2_http2_settings *http2_settings, size_t initial_window_size) {
  http2_settings->initial_window_size = initial_window_size;
}

void ezgrpc2_http2_settings_set_max_frame_size(ezgrpc2_http2_settings *http2_settings, size_t max_frame_size) {
  http2_settings->max_frame_size = max_frame_size;
}

void ezgrpc2_http2_settings_set_max_concurrent_streams(ezgrpc2_http2_settings *http2_settings, size_t max_concurrent_streams) {
  http2_settings->max_concurrent_streams = max_concurrent_streams;
}

void ezgrpc2_http2_settings_set_max_paths(ezgrpc2_http2_settings *http2_settings, size_t max_paths) {
  http2_settings->max_paths = max_paths;
}
//void ezgrpc2_http2_settings_set_write_callback(ezgrpc2_http2_settings *http2_settings,
//    ssize_t (*write)(void *sock, void *buf, size_t len, int flags)
//    ){
//  http2_settings->write_cb = write;
//}
//
//void ezgrpc2_http2_settings_set_read_callback(ezgrpc2_http2_settings *http2_settings,
//    ssize_t (*read)(void *sock, void *buf, size_t len, int flags)
//    ){
//  http2_settings->read_cb = read;
//}

void ezgrpc2_http2_settings_free(ezgrpc2_http2_settings *http2_settings) {
  free(http2_settings);
}
