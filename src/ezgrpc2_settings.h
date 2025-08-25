#ifndef EZGRPC2_SETTINGS_H
#define EZGRPC2_SETTINGS_H

typedef enum ezgrpc2_settings_type_t ezgrpc2_settings_type_t;

enum ezgrpc2_settings_type_t {
  NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE = 0,
  NGHTTP2_SETTINGS_MAX_FRAME_SIZE = 1,
  NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS = 3
};

typedef struct ezgrpc2_settings_t ezgrpc2_settings_t;

#endif
