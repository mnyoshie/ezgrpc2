#ifndef EZGRPC2_INTERNAL_NGHTTP2_CALLBACKS_H
#define EZGRPC2_INTERNAL_NGHTTP2_CALLBACKS_H
#include <nghttp2/nghttp2.h>
void server_setup_ngcallbacks(nghttp2_session_callbacks *ngcallbacks);
nghttp2_ssize data_source_read_callback2(nghttp2_session *session,
                                         i32 stream_id, u8 *buf, size_t buf_len,
                                         u32 *data_flags,
                                         nghttp2_data_source *source,
                                         void *user_data);
#endif
