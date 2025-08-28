#ifndef EZGRPC2_SERVER_SETTINGS_STRUCT_H
#define EZGRPC2_SERVER_SETTINGS_STRUCT_H
struct ezgrpc2_server_settings_t {
  ssize_t (*write_cb)(void *sock, void *buf, size_t len, int flags); 
  ssize_t (*read_cb)(void *sock, void *buf, size_t len, int flags); 
  size_t max_connections;
};
#endif
