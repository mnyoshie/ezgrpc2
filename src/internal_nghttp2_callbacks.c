#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "core.h"
#include "ezgrpc2_event.h"
#include "ezgrpc2_grpc_status.h"

#include "internal_helpers.h"
#include "internal_nghttp2_callbacks.h"

#define atlog(...) (void)0

extern ezgrpc2_event *event_new(ezgrpc2_event_type type, ezgrpc2_session_uuid *session_uuid, ...);

static int list_cmp_ezstream_id(const void *data,const void *userdata) {
  return ((ezgrpc2_stream*)data)->stream_id != *(i32*)userdata;
}

static size_t parse_grpc_message(void *data, size_t len, ezgrpc2_list *lmessages) {
  i8 *wire = data;

  size_t seek, last_seek = 0;
  for (seek = 0; seek < len;) {
    if (seek + 1 > len) {
      return last_seek;
    }
    i8 is_compressed = wire[seek];
    seek += 1;

    if (seek + 4 > len) {
      /* atlog("exceeded length prefixed\n"); */
      return last_seek;
    }
    u32 msg_len = ntohl(uread_u32(wire + seek));
    seek += 4;
    void *msg_data = wire + seek;

    if (seek + msg_len <= len) {
      last_seek = seek;
      ezgrpc2_message *msg = ezgrpc2_message_new(is_compressed, msg_data, msg_len);
      assert(msg != NULL);

      ezgrpc2_list_push_back(lmessages, msg);
    }
    seek += msg_len;
  }

  return last_seek;
}

/* ----------- BEGIN NGHTTP2 CALLBACKS ------------------*/


nghttp2_ssize data_source_read_callback2(nghttp2_session *ngsession,
                                         i32 stream_id, u8 *buf, size_t buf_len,
                                         u32 *data_flags,
                                         nghttp2_data_source *source,
                                         void *user_data) {
#ifdef EZGRPC2_DEBUG
  ezgrpc2_session *ezsession = user_data;
#endif
  (void)stream_id;
  (void)user_data;
  if (buf_len < 5) {
    EZGRPC2_LOG_DEBUG(ezsession->server, "error buf_len < 5\n");
    return NGHTTP2_INTERNAL_ERROR;
  }
//  ezgrpc2_stream *ezstream = nghttp2_session_get_stream_user_data(session, stream_id);
//  assert(ezstream != NULL);
  ezgrpc2_stream *ezstream = source->ptr;
  ezgrpc2_list *lmessages = ezstream->lqueue_omessages;
  ezgrpc2_message *msg;


  size_t buf_seek = 0;
  while (1) {
    if ((msg = ezgrpc2_list_peek_front(lmessages)) == NULL) {
      *data_flags = NGHTTP2_DATA_FLAG_EOF | NGHTTP2_DATA_FLAG_NO_END_STREAM;
      goto exit;
    }
    size_t rem;
    if (ezstream->is_trunc) {
      assert(buf_len >= buf_seek);
      if ((rem = buf_len - buf_seek) == 0) {
        /* buf is fully written */
        goto exit;
      }
      assert(msg->len > ezstream->trunc_seek);
      char is_fit = msg->len - ezstream->trunc_seek <= rem;
      size_t to_write = is_fit ? msg->len - ezstream->trunc_seek :
        rem;
      memcpy(buf + buf_seek, msg->data + ezstream->trunc_seek, to_write);
      buf_seek += to_write;

      if (is_fit) {
      /* the whole message can fit! */
        (void)ezgrpc2_list_pop_front(lmessages);
        ezstream->is_trunc = 0;
        ezstream->trunc_seek = 0;
        continue;
      }
      else /*if(msg->len > rem)*/ {
        ezstream->trunc_seek += rem;
        goto exit;
      }
    }
    /* if we can fit 5 bytes or more to the buffer */
    if (buf_seek + 5 <= buf_len)  {
      buf[buf_seek++] = msg->is_compressed;
      uint32_t len = htonl(msg->len);
      memcpy(buf + buf_seek, &len, 4);
      buf_seek += 4;
      ezstream->is_trunc = 1;
      ezstream->trunc_seek = 0;
      continue;
    }
    break;
  } 
exit:
  EZGRPC2_LOG_DEBUG(ezsession->server, "read done %d bytes\n", buf_seek);
 // *data_flags = NGHTTP2_DATA_FLAG_EOF | NGHTTP2_DATA_FLAG_NO_END_STREAM;
  return buf_seek;
}







static nghttp2_ssize ngsend_callback(nghttp2_session *session, const u8 *data,
                               size_t length, int flags, void *user_data) {
  (void)session;
  ezgrpc2_session *ezsession = user_data;

  EZSSIZE ret = send(ezsession->sockfd, data, length, 0);
  if (ret < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) return NGHTTP2_ERR_WOULDBLOCK;
      EZGRPC2_LOG_DEBUG(ezsession->server, "send: %s\n", strerror(errno));

    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }

  EZGRPC2_LOG_TRACE(ezsession->server, "read %zu bytes\n", ret);
  return ret;
}








static nghttp2_ssize ngrecv_callback(nghttp2_session *session, u8 *buf, size_t length,
                               int flags, void *user_data) {
  ezgrpc2_session *ezsession = user_data;

#ifdef _WIN32
  int ret = recv(ezsession->sockfd, buf, length, 0);
  if (ret == SOCKET_ERROR) {
    int wsa;
    switch ((wsa = WSAGetLastError())) {
      case WSAEWOULDBLOCK:
      case WSAEINTR:
        return NGHTTP2_ERR_WOULDBLOCK;
      default:
        atlog("failure errno %d\n", wsa);
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
  }
#else
  ssize_t ret = recv(ezsession->sockfd, buf, length, 0);
  if (ret == -1) {
    switch (errno) {
      case EWOULDBLOCK:
        return NGHTTP2_ERR_WOULDBLOCK;
      default:
        EZGRPC2_LOG_DEBUG(ezsession->server, "recv: %s\n", strerror(errno));
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
  }
#endif

  EZGRPC2_LOG_TRACE(ezsession->server, "read %zu bytes\n", ret);
  return ret;
}







/* we're beginning to receive a header; open up a memory for the new stream */
static int on_begin_headers_callback(nghttp2_session *session,
                                     const nghttp2_frame *frame,
                                     void *user_data) {

  ezgrpc2_session *ezsession = user_data;
  ezgrpc2_stream *ezstream = stream_new(frame->hd.stream_id);

  ezstream->recv_len = 0;
  ezstream->recv_data = malloc(ezsession->server->http2_settings.initial_window_size);

  ezgrpc2_list_push_back(ezsession->lstreams, ezstream);
  nghttp2_session_set_stream_user_data(session, frame->hd.stream_id, ezstream);

  EZGRPC2_LOG_TRACE(ezsession->server, "stream id %d\n", frame->hd.stream_id);
  return 0;
}









/* ok, here's the header name and value. this will be called from time to time
 */
static int on_header_callback(nghttp2_session *session,
                              const nghttp2_frame *frame, const u8 *name,
                              size_t namelen, const u8 *value, size_t valuelen,
                              u8 flags, void *user_data) {

  (void)flags;
  ezgrpc2_session *ezsession = user_data;
  ezgrpc2_stream *ezstream = nghttp2_session_get_stream_user_data(ezsession->ngsession, frame->hd.stream_id);
  if (frame->hd.type != NGHTTP2_HEADERS)
    return 0;

  if (ezstream->nb_headers + 1 >= EZGRPC2_STREAM_MAX_HEADERS) {
    EZGRPC2_LOG_ERROR(ezsession->server, "stream max headers exceeded\n");
    return 1;
  }
  ezstream->headers[ezstream->nb_headers].name = malloc(namelen + 1);
  ezstream->headers[ezstream->nb_headers].value = malloc(valuelen + 1);
  ezstream->headers[ezstream->nb_headers].namelen = namelen;
  ezstream->headers[ezstream->nb_headers].valuelen = valuelen;
  memcpy(ezstream->headers[ezstream->nb_headers].name, name, namelen + 1);
  memcpy(ezstream->headers[ezstream->nb_headers].value, value, valuelen + 1);
  ezstream->nb_headers++;

  i8 *method = ":method";
  i8 *scheme = ":scheme";
  i8 *path = ":path";
  /* clang-format off */
#define EZSTRCMP(str1, str2) !(strlen(str1) == str2##len && !strcmp(str1, (void *)str2))
  if (!EZSTRCMP(method, name))
    ezstream->is_method_post = !EZSTRCMP("POST", value);
  else if (!EZSTRCMP(scheme, name))
    ezstream->is_scheme_http = !EZSTRCMP("http", value);
  else if (!EZSTRCMP(path, name)) {
    ezgrpc2_stream *ezstream = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
    if (ezstream == NULL) {
      atlog("couldn't find stream %d\n", frame->hd.stream_id);
      return 1;
    }
    for (size_t i = 0; i < ezsession->nb_paths; i++) 
      if (!EZSTRCMP(ezsession->paths[i].path, value)) {
        ezstream->path = &ezsession->paths[i];
        ezstream->path_index = i;
        EZGRPC2_LOG_TRACE(ezsession->server, "found service %s\n", ezsession->paths[i].path);
        break;
      }
    if (ezstream->path == NULL)
      atlog("path not found %s\n", value);
  }
  else if (!EZSTRCMP("content-type", name)) {
    int is_gt = valuelen >= 16;
    ezstream->hcontent_type = strdup((char*)value);
    ezstream->is_content_grpc = is_gt ? !strncmp("application/grpc", (void *)value, 16) : 0;
  }
  else if (!EZSTRCMP("grpc-encoding", name)) {
    ezstream->hgrpc_encoding = strdup((char*)value);
  }
  else if (!EZSTRCMP("grpc-accept-encoding", name)) {
    ezstream->hgrpc_accept_encoding = strdup((char*)value);
  }
  else if (!EZSTRCMP("grpc-timeout", name)) {
    ezstream->hgrpc_timeout = strdup((char*)value);
  }
  else if (!EZSTRCMP("te", name)) {
    ezstream->is_te_trailers = !EZSTRCMP("trailers", value);
  }
  else {
    ;
  }
#undef EZSTRCMP
  /* clang-format on */

  return 0;
}










static inline int on_frame_recv_headers(ezgrpc2_session *ezsession, const nghttp2_frame *frame) {
  ezgrpc2_stream *ezstream = nghttp2_session_get_stream_user_data(ezsession->ngsession, frame->hd.stream_id);
  assert(ezstream != NULL);
  if (!(frame->hd.flags & NGHTTP2_FLAG_END_HEADERS)){
    return 0;
  }
  /* if we've received an end stream in headers frame. send an RST. we
   * expected a data */
#if 0
  if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) {
    nghttp2_submit_rst_stream(ezsession->ngsession, NGHTTP2_FLAG_NONE,
                              frame->hd.stream_id, 1);
    return 0;
  }
#endif
 
     // (void*)&frame->hd.stream_id);
  if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM && !ezstream->is_content_grpc) {
    nghttp2_nv nva[1] =
    {
        {
          .name = (void *)":status", .namelen= 7,
          .value = (void *)"415", .valuelen= 3,
          .flags=NGHTTP2_NV_FLAG_NONE
        },
    };
    if (nghttp2_submit_headers(ezsession->ngsession, NGHTTP2_FLAG_END_STREAM,
        ezstream->stream_id, NULL, nva, 1, NULL)){assert(0);}

    return 0;
  }

  if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) {
    EZGRPC2_LOG_DEBUG(ezsession->server, "received a header with an end stream. sending rst\n", frame->hd.stream_id);
    nghttp2_submit_rst_stream(ezsession->ngsession, NGHTTP2_FLAG_NONE,
                              frame->hd.stream_id, 1);
    return 0;
  }

  if (!(ezstream->is_method_post && ezstream->is_scheme_http &&
        ezstream->is_te_trailers && ezstream->is_content_grpc)) {
    EZGRPC2_LOG_TRACE(ezsession->server, "is_method_post %d && is_scheme_http %d && is_te_trailers %d && is_content_grpc %d. sending rst\n", ezstream->is_method_post, ezstream->is_scheme_http, ezstream->is_te_trailers, ezstream->is_content_grpc);
    nghttp2_submit_rst_stream(ezsession->ngsession, NGHTTP2_FLAG_NONE,
                              ezstream->stream_id, 1);
    return 0;
  }
  nghttp2_nv nva[2] =
  {
      {
        .name = (void *)":status", .namelen= 7,
        .value = (void *)"200", .valuelen= 3,
        .flags=NGHTTP2_NV_FLAG_NONE
      },
      {
        .name = (void *)"content-type", .namelen = 12,
        .value = (void *)"application/grpc", .valuelen = 16,
        .flags=NGHTTP2_NV_FLAG_NONE
      },
  };

  if (nghttp2_submit_headers(ezsession->ngsession, 0, ezstream->stream_id, NULL, nva, 2,
                           NULL)) {
    EZGRPC2_LOG_TRACE(ezsession->server, "nghttp2_submit_headers failed\n");
    return 1;
  }

  if (ezstream->path == NULL) {
    atlog("path null, submitting req\n");
    int status = EZGRPC2_GRPC_STATUS_UNIMPLEMENTED;
    char grpc_status[32] = {0};
    int len = snprintf(grpc_status, 31, "%d",(int) status);
    i8 *grpc_message = ezgrpc2_grpc_status_strstatus(status);
    nghttp2_nv trailers[] = {
        {(void *)"grpc-status", (void *)grpc_status, 11, len},
        {(void *)"grpc-message", (void *)grpc_message, 12,
         strlen(grpc_message)},
    };
    nghttp2_submit_trailer(ezsession->ngsession, frame->hd.stream_id, trailers, 2);
    EZGRPC2_LOG_TRACE(ezsession->server, "path null. trailer submitted\n");
   // nghttp2_session_send(ezsession->ngsession);
   // ezgrpc2_session_end_stream(ezsession, ezstream->stream_id, EZGRPC2_GRPC_STATUS_UNIMPLEMENTED);
    return 0;
  }

  EZGRPC2_LOG_TRACE(ezsession->server, "ret id %d\n", frame->hd.stream_id);
  return 0;
}








static inline int on_frame_recv_settings(ezgrpc2_session *ezsession, const nghttp2_frame *frame) {
  if (frame->hd.stream_id != 0){
    EZGRPC2_LOG_DEBUG(ezsession->server, "received a SETTINGS frame at stream_id 0. immediately returning\n");
    return 0;
  }

  if (!(frame->settings.hd.flags & NGHTTP2_FLAG_ACK)) {
    // apply settings
//    for (size_t i = 0; i < frame->settings.niv; i++) {
//      switch (frame->settings.iv[i].settings_id) {
//        case NGHTTP2_SETTINGS_HEADER_TABLE_SIZE:
//          ezsession->csettings.header_table_size =
//              frame->settings.iv[i].value;
//          break;
//        case NGHTTP2_SETTINGS_ENABLE_PUSH:
//          ezsession->csettings.enable_push = frame->settings.iv[i].value;
//          break;
//        case NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS:
//          ezsession->csettings.max_concurrent_streams =
//              frame->settings.iv[i].value;
//          break;
//        case NGHTTP2_SETTINGS_MAX_FRAME_SIZE:
//          ezsession->csettings.max_frame_size = frame->settings.iv[i].value;
//          // case NGHTTP2_SETTINGS_FLOW_CONTROL:
//          // ezstream->flow_control =  frame->settings.iv[i].value;
//          break;
//        default:
//          break;
//      };
//    }
  } else if (frame->settings.hd.flags & NGHTTP2_FLAG_ACK &&
             frame->settings.hd.length == 0) {
    /* OK. The client acknowledge our settings. do nothing */
  } else {
    atlog("sent an rst\n", frame->hd.stream_id);
    nghttp2_submit_rst_stream(ezsession->ngsession, NGHTTP2_FLAG_NONE,
                              frame->hd.stream_id,
                              1);  // XXX send appropriate code
  }
  return 0;
}










static inline int on_frame_recv_data(ezgrpc2_session *ezsession, const nghttp2_frame *frame) {

  ezgrpc2_stream *ezstream = nghttp2_session_get_stream_user_data(ezsession->ngsession, frame->hd.stream_id);
  if (ezstream == NULL) {
    atlog("stream %d not found. sending rst\n", frame->hd.stream_id);
    nghttp2_submit_rst_stream(ezsession->ngsession, NGHTTP2_FLAG_NONE,
                              frame->hd.stream_id, 1);
    return 0;
  }
  if (ezstream->path == NULL)
    return 0;

  ezgrpc2_list *lmessages = ezgrpc2_list_new(NULL);

  if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM && ezstream->recv_len == 0) {
    /* In scenarios where the Request stream needs to be closed but no data remains
     * to be sent implementations MUST send an empty DATA frame with this flag set. */
    atlog("event message %zu\n", ezgrpc2_list_count(lmessages));
    ezgrpc2_event *event = event_new(
      EZGRPC2_EVENT_MESSAGE,
      &ezsession->session_uuid,
      ((ezgrpc2_event_message) {
        .lmessages = lmessages,
        .end_stream = 1,
        .stream_id = frame->hd.stream_id,
      })
    );

    ezgrpc2_list_push_back(ezsession->server->levents, event);
    return 0;
  }

  size_t lseek = parse_grpc_message(ezstream->recv_data, ezstream->recv_len, lmessages);
  if (lseek != 0) {
    assert(lseek <= ezstream->recv_len);
    EZGRPC2_LOG_TRACE(ezsession->server, "event message %zu\n", ezgrpc2_list_count(lmessages));
    memcpy(ezstream->recv_data, ezstream->recv_data + lseek, ezstream->recv_len - lseek);
    ezstream->recv_len -= lseek;
    ezgrpc2_event *event = event_new(
      EZGRPC2_EVENT_MESSAGE,
      &ezsession->session_uuid,
      ((ezgrpc2_event_message) {
        .lmessages = lmessages,
        .end_stream = frame->hd.flags & NGHTTP2_FLAG_END_STREAM && ezstream->recv_len == 0,
        .stream_id = frame->hd.stream_id,
      })
    );
    event->message.path_index = ezstream->path_index;

    ezgrpc2_list_push_back(ezsession->server->levents, event);
  }
  if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM && ezstream->recv_len != 0) {
    /* we've receaived an end stream but last message is truncated */
    atlog("event dataloss %zu\n", ezgrpc2_list_count(lmessages));
    ezgrpc2_event *event = event_new(
      EZGRPC2_EVENT_DATALOSS,
      &ezsession->session_uuid,
      ((ezgrpc2_event_dataloss) {
        .stream_id = ezstream->stream_id,
      })
    );
    event->dataloss.path_index = ezstream->path_index;
  
    ezgrpc2_list_push_back(ezsession->server->levents, event);
    //event->message.end_stream = !!(frame->hd.flags & NGHTTP2_FLAG_END_STREAM);
  }
  return 0;
}






static int on_frame_send_callback(nghttp2_session *session,
                                  const nghttp2_frame *frame, void *user_data) {
#ifdef EZGRPC2_TRACE
  ezgrpc2_session *ezsession = user_data;
#endif

  switch (frame->hd.type) {
    case NGHTTP2_SETTINGS:
      EZGRPC2_LOG_TRACE(ezsession->server, "> FRAME[SETTINGS, sid=%d, ack=%d]\n", frame->hd.stream_id, !!(frame->hd.flags & NGHTTP2_FLAG_ACK));
      return 0;
    case NGHTTP2_HEADERS:
      EZGRPC2_LOG_TRACE(ezsession->server, "> FRAME[HEADERS, sid=%d, eos=%d]\n", frame->hd.stream_id, !!(frame->hd.flags & NGHTTP2_FLAG_END_STREAM));
      for (size_t i = 0; i < frame->headers.nvlen; i++)
        EZGRPC2_LOG_TRACE(ezsession->server, "  %s: %s\n", frame->headers.nva[i].name, frame->headers.nva[i].value);
      return 0;
    case NGHTTP2_DATA:
      EZGRPC2_LOG_TRACE(ezsession->server, "> FRAME[DATA, sid=%d, eos=%d, len=%zu]\n", frame->hd.stream_id, !!(frame->hd.flags & NGHTTP2_FLAG_END_STREAM), frame->hd.length);
      return 0;
    case NGHTTP2_WINDOW_UPDATE:
      EZGRPC2_LOG_TRACE(ezsession->server, "> FRAME[WINDOW_UPDATE, sid=%d, eos=%d, len=%zu, wsi=%d]\n", frame->hd.stream_id, !!(frame->hd.flags & NGHTTP2_FLAG_END_STREAM), frame->hd.length, frame->window_update.window_size_increment);
      // printf("frame window update %d\n",
      // frame->window_update.window_size_increment); int res;
      //    if ((res = nghttp2_session_set_local_window_size(session,
      //    NGHTTP2_FLAG_NONE, frame->hd.stream_id,
      //    frame->window_update.window_size_increment)) != 0)
      //      assert(0);
      break;
    default:
      break;
  }
  return 0;
}







static int on_frame_recv_callback(nghttp2_session *session,
                                  const nghttp2_frame *frame, void *user_data) {
  ezgrpc2_session *ezsession = user_data;
  ezgrpc2_stream *ezstream = nghttp2_session_get_stream_user_data(ezsession->ngsession, frame->hd.stream_id);
  switch (frame->hd.type) {
    case NGHTTP2_SETTINGS:
      EZGRPC2_LOG_TRACE(ezsession->server, "< FRAME[SETTINGS, sid=%d, ack=%d]\n", frame->hd.stream_id, !!(frame->hd.flags & NGHTTP2_FLAG_ACK));
      return on_frame_recv_settings(ezsession, frame);
    case NGHTTP2_HEADERS:
      EZGRPC2_LOG_TRACE(ezsession->server, "< FRAME[HEADERS, sid=%d, eos=%d]\n", frame->hd.stream_id, !!(frame->hd.flags & NGHTTP2_FLAG_END_STREAM));
      for (size_t i = 0; i < ezstream->nb_headers; i++)
        EZGRPC2_LOG_TRACE(ezsession->server, "  %s: %s\n", ezstream->headers[i].name, ezstream->headers[i].value);
      return on_frame_recv_headers(ezsession, frame);
    case NGHTTP2_DATA:
      EZGRPC2_LOG_TRACE(ezsession->server, "< FRAME[DATA, sid=%d, eos=%d, len=%zu]\n", frame->hd.stream_id, !!(frame->hd.flags & NGHTTP2_FLAG_END_STREAM), frame->hd.length);
      return on_frame_recv_data(ezsession, frame);
    case NGHTTP2_WINDOW_UPDATE:
      EZGRPC2_LOG_TRACE(ezsession->server, "< FRAME[WINDOW_UPDATE, sid=%d, eos=%d, len=%zu, wsi=%d]\n", frame->hd.stream_id, !!(frame->hd.flags & NGHTTP2_FLAG_END_STREAM), frame->hd.length, frame->window_update.window_size_increment);
      // printf("frame window update %d\n",
      // frame->window_update.window_size_increment); int res;
      //    if ((res = nghttp2_session_set_local_window_size(session,
      //    NGHTTP2_FLAG_NONE, frame->hd.stream_id,
      //    frame->window_update.window_size_increment)) != 0)
      //      assert(0);
      break;
    default:
      break;
  }
  return 0;
}









static int on_data_chunk_recv_callback(nghttp2_session *session, u8 flags,
                                       i32 stream_id, const u8 *data,
                                       size_t len, void *user_data) {
  ezgrpc2_session *ezsession = user_data;
  ezgrpc2_stream *ezstream = nghttp2_session_get_stream_user_data(session, stream_id);
  //ezgrpc2_stream *ezstream = ezgrpc2_list_find(&ezsession->lstreams, ezgrpc2_list_cmp_ezstream_id, &stream_id);
  if (ezstream == NULL) {
    atlog("stream_id %d doesn't exist\n", stream_id);
    return 0;
  }


  /* prevent an attacker from pushing large POST data */
  if ((ezstream->recv_len + len) > ezsession->server->http2_settings.initial_window_size) {
    nghttp2_submit_rst_stream(ezsession->ngsession, NGHTTP2_FLAG_NONE,
                              stream_id, 1);  // XXX send appropriate code
    EZGRPC2_LOG_ERROR(
      ezsession->server, "POST data exceeds maximum allowed size, killing stream %d\n",
      stream_id
    );
    return 1;
  }


  memcpy(ezstream->recv_data + ezstream->recv_len, data, len);
  ezstream->recv_len += len;
  EZGRPC2_LOG_TRACE(ezsession->server, "data chunk recv %zu bytes at stream %d\n", len, stream_id);

  return 0;
}

static int on_stream_close_callback(nghttp2_session *session, i32 stream_id,
                             u32 error_code, void *user_data) {
  atlog("stream %d closed\n", stream_id);
  ezgrpc2_session *ezsession = user_data;
  ezgrpc2_stream *ezstream = nghttp2_session_get_stream_user_data(session, stream_id);
  assert(ezstream == ezgrpc2_list_remove(ezsession->lstreams, list_cmp_ezstream_id, &stream_id));
  stream_free(ezstream);

  return 0;
}


/* ----------- END NGHTTP2 CALLBACKS ------------------*/

void server_setup_ngcallbacks(nghttp2_session_callbacks *ngcallbacks) {
  /* clang-format off */
  nghttp2_session_callbacks_set_send_callback(ngcallbacks, ngsend_callback);
  nghttp2_session_callbacks_set_recv_callback(ngcallbacks, ngrecv_callback);

  /* prepares and allocates memory for the incoming HEADERS frame */
  nghttp2_session_callbacks_set_on_begin_headers_callback(ngcallbacks, on_begin_headers_callback);
  /* we received that HEADERS frame. now here's the http2 fields */
  nghttp2_session_callbacks_set_on_header_callback(ngcallbacks, on_header_callback);
  /* we received a frame. do something about it */
  nghttp2_session_callbacks_set_on_frame_recv_callback(ngcallbacks, on_frame_recv_callback);
  nghttp2_session_callbacks_set_on_frame_send_callback(ngcallbacks, on_frame_send_callback);

  nghttp2_session_callbacks_set_on_data_chunk_recv_callback(ngcallbacks, on_data_chunk_recv_callback);
  nghttp2_session_callbacks_set_on_stream_close_callback(ngcallbacks, on_stream_close_callback);
 // nghttp2_session_callbacks_set_send_data_callback(ngcallbacks, send_data_callback);
  //  nghttp2_session_callbacks_set_before_frame_send_callback(callbacks,
  //  before_frame_send_callback);

  /* clang-format on */

}
