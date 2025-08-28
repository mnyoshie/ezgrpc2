#ifndef EZGRPC2_HTTP2_SETTINGS_STRUCT_H
#define EZGRPC2_HTTP2_SETTINGS_STRUCT_H
#include <stdlib.h>
struct ezgrpc2_http2_settings_t {
  size_t initial_window_size;
  size_t max_frame_size;
  size_t max_concurrent_streams;
  size_t max_sessions;
};
#endif
