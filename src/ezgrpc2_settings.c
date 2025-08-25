#include "ezgrpc2_settings.h"

struct ezgrpc2_settings_t {
  size_t initial_window_size;
  size_t max_frame_size;
  size_t max_concurrent_streams;
};

ezgrpc2_settings_t *ezgrpc2_settings_new(NULL){
  return malloc(sizeof(ezgrpc2_settings_t));
}

void ezgrpc2_settings_set(

void
