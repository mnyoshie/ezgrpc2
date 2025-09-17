#ifndef EZGRPC2_SERVER_SETTINGS_H
#define EZGRPC2_SERVER_SETTINGS_H
#include <stdlib.h>
typedef struct ezgrpc2_server_settings_t ezgrpc2_server_settings_t;
void ezgrpc2_server_settings_set_max_connections(ezgrpc2_server_settings_t *server_settings, size_t max_connections);
void ezgrpc2_server_settings_set_logging_level(ezgrpc2_server_settings_t *server_settings, uint32_t logging_level);
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

ezgrpc2_server_settings_t *ezgrpc2_server_settings_new(void *unused);
void ezgrpc2_server_settings_free(ezgrpc2_server_settings_t *server_settings);
#endif
