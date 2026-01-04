#ifndef EZGRPC2_SERVER_SETTINGS_STRUCT_H
#define EZGRPC2_SERVER_SETTINGS_STRUCT_H
#include <stdlib.h>
#include <stdbool.h>
struct ezgrpc2_server_settings {
  ssize_t (*write_cb)(void *sock, void *buf, size_t len, int flags); 
  ssize_t (*read_cb)(void *sock, void *buf, size_t len, int flags); 
  size_t max_connections;
  uint32_t logging_level;
  size_t arena_events_size;
  size_t arena_messages_size;
  FILE *logging_fp;
};
#endif
