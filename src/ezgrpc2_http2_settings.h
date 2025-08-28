#ifndef EZGRPC2_HTTP2_SETTINGS_H
#define EZGRPC2_HTTP2_SETTINGS_H
typedef struct ezgrpc2_http2_settings_t ezgrpc2_http2_settings_t;
void ezgrpc2_http2_settings_set_initial_window_size(ezgrpc2_http2_settings_t *http2_settings, size_t initial_window_size);
void ezgrpc2_http2_settings_set_max_frame_size(ezgrpc2_http2_settings_t *http2_settings, size_t max_frame_size);
void ezgrpc2_http2_settings_set_max_concurrent_streams(ezgrpc2_http2_settings_t *http2_settings, size_t max_concurrent_streams);
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

ezgrpc2_http2_settings_t *ezgrpc2_http2_settings_new(void *unused);
void ezgrpc2_http2_settings_free(ezgrpc2_http2_settings_t *http2_settings);
#endif
