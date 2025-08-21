/* ezgrpc2.c - a grpc server without the extra fancy
 * features
 *
 *
 *            _____ _____     ____  ____   ____ _____
 *           | ____|__  /__ _|  _ \|  _ \ / ___|___  \
 *           |  _|   / // _` | |_) | |_) | |     __/  |
 *           | |___ / /| (_| |  _ <|  __/| |___ / ___/
 *           |_____/____\__, |_| \_\_|    \____|______|
 *                      |___/
 *
 *
 * Copyright (c) 2023-2025 M. N. Yoshie & Al-buharie Amjari
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 *   2. Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "ezgrpc2.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <openssl/rand.h>
#include <nghttp2/nghttp2.h>

#include "ezgrpc2_event.h"


//_Static_assert(UUID_STR_LEN == EZGRPC2_SESSION_UUID_LEN, "");

#include "ansicolors.h"

#ifdef _WIN32
#  include <ws2tcpip.h>

#  define ssize_t int
#  define SHUT_RDWR SD_BOTH
#  define POLLRDHUP 0

static char *strndup(char *c, size_t n) {
  size_t l;
  for (l = 0; l < n && c[l];) l++;
  char *r = calloc(1, l+1);
  memcpy(r, c, l);
  return r;
}

/* winsock send/recv returns int instead of ssize_t */
#define EZSSIZE           int
#define EZSOCKET          SOCKET
#define EZPOLL            WSAPoll
#define EZNFDS            ULONG
#define EZINVALID_SOCKET  INVALID_SOCKET
#define EZSOCKET_ERROR    SOCKET_ERROR
#define EZSOCKLEN         int
#else /* __unix__ */
#  include <arpa/inet.h>
#  include <byteswap.h>
#  include <fcntl.h>
#  include <netdb.h>
#  include <poll.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <sys/socket.h>
#  include <sys/types.h>

#define EZSSIZE           ssize_t
#define EZSOCKET          int
#define EZPOLL            poll
#define EZNFDS            nfds_t
#define EZINVALID_SOCKET  (-1)
#define EZSOCKET_ERROR    (-1)
#define EZSOCKLEN         socklen_t
#endif /* _WIN32/__unix__ */

#define logging_fp stderr
// int logging_level;

//#define EZENABLE_IPV6
#define EZWINDOW_SIZE (1<<20)

#define atlog(fmt, ...) ezlog("@ %s: " fmt, __func__, ##__VA_ARGS__)
//#define atlog(fmt, ...) (void)0

//#define EZENABLE_DEBUG
#define ezlogm(...) \
  ezlogf(ezsession, __FILE__, __LINE__, __VA_ARGS__)


/* stores the values of a SETTINGS frame */
typedef struct ezgrpc_settingsf_t ezgrpc_settingsf_t;
struct ezgrpc_settingsf_t {
  u32 header_table_size;
  u32 enable_push;
  u32 max_concurrent_streams;
  u32 flow_control;
  u32 max_frame_size;
};


typedef struct ezgrpc2_stream_t ezgrpc2_stream_t;
struct ezgrpc2_stream_t {
  i32 stream_id;

  /* the time the stream is received by the server in
   * unix epoch */
  u64 time;
  ezgrpc2_list_t *lheaders;

  i8 is_method_post : 1;
  i8 is_scheme_http : 1;
  /* just a bool. if `te` is a trailer, or `te` does exists. */
  i8 is_te_trailers : 1;
  /* just a bool. if content type is application/grpc* */
  i8 is_content_grpc : 1;

  i8 is_trunc : 1;

  /* stores `:path` */
  ezgrpc2_path_t *path;

  /* recv_data */
  size_t recv_len;
  u8 *recv_data;

  size_t trunc_seek;
  ezgrpc2_list_t *lqueue_omessages;


};

typedef struct ezgrpc2_session_t ezgrpc2_session_t;
struct ezgrpc2_session_t {
  //u8 session_id[32];
//  char session_uuid[UUID_STR_LEN];
  ezgrpc2_session_uuid_t *session_uuid;

  EZSOCKET sockfd;
  struct sockaddr_storage sockaddr;
  EZSOCKLEN socklen;

  nghttp2_session *ngsession;

  /* an ASCII string */
  char client_addr[64];
  u16 client_port;

  char *server_addr;
  u16 *server_port;

  size_t nb_paths;
  ezgrpc2_path_t *paths;


  /* settings requested by the client. to be set when a SETTINGS
   * frame is received.
   */
  ezgrpc_settingsf_t csettings;

  /* A pointer to the settings in ezgrpc2_server_t.settings
   */
  ezgrpc_settingsf_t *ssettings;

  size_t nb_open_streams;
  /* the streams in a linked lists. allocated when we are
   * about to receive a HEADERS frame */
  ezgrpc2_list_t *lstreams;

  //int pfd[2];
};

/* This struct is private. Please use the auxilliary functions below it */
struct ezgrpc2_server_t {
  nghttp2_session *ngsession;
  u16 ipv4_port;
  u16 ipv6_port;

  /* ASCII strings which represent where address to bind to */
  char *ipv4_addr;
  char *ipv6_addr;

  EZNFDS nb_fds;

  /* fds[0] reserve for ipv4 listen sockfd */
  /* fds[1] reserve for ipv6 listen sockfd */
  /* the rest are for client use */
  struct pollfd *fds;

  size_t nb_sessions;
  ezgrpc2_session_t *sessions;


  /* default server settings to be sent */
  ezgrpc_settingsf_t settings;

};

i8 *ezgrpc2_status_strstatus(ezgrpc2_status_t status) {
  switch (status) {
    case EZGRPC2_STATUS_OK:
      return "ok";
    case EZGRPC2_STATUS_CANCELLED:
      return "cancelled";
    case EZGRPC2_STATUS_UNKNOWN:
      return "unknown";
    case EZGRPC2_STATUS_INVALID_ARGUMENT:
      return "invalid argument";
    case EZGRPC2_STATUS_DEADLINE_EXCEEDED:
      return "deadline exceeded";
    case EZGRPC2_STATUS_NOT_FOUND:
      return "not found";
    case EZGRPC2_STATUS_ALREADY_EXISTS:
      return "already exists";
    case EZGRPC2_STATUS_PERMISSION_DENIED:
      return "permission denied";
    case EZGRPC2_STATUS_UNAUTHENTICATED:
      return "unauthenticated";
    case EZGRPC2_STATUS_RESOURCE_EXHAUSTED:
      return "resource exhausted";
    case EZGRPC2_STATUS_FAILED_PRECONDITION:
      return "failed precondition";
    case EZGRPC2_STATUS_OUT_OF_RANGE:
      return "out of range";
    case EZGRPC2_STATUS_UNIMPLEMENTED:
      return "unimplemented";
    case EZGRPC2_STATUS_INTERNAL:
      return "internal";
    case EZGRPC2_STATUS_UNAVAILABLE:
      return "unavailable";
    case EZGRPC2_STATUS_DATA_LOSS:
      return "data loss";
    default:
      return "(null)";
  }
}

static void dump_decode_binary(i8 *data, size_t len) {
  i8 look_up[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  for (size_t i = 0; i < len; i++) {
    putchar(look_up[(data[i] >> 4) & 0x0f]);
    putchar(look_up[data[i] & 0x0f]);
    putchar(' ');
    if (i == 7) putchar(' ');
  }
}

static void dump_decode_ascii(i8 *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 0x20) {
      putchar('.');
    } else if (data[i] < 0x7f)
      putchar(data[i]);
    else
      putchar('.');
  }
}

void ezdump(void *vdata, size_t len) {
  i8 *data = vdata;
  if (len == 0) return;

  size_t cur = 0;

  for (; cur < 16 * (len / 16); cur += 16) {
    dump_decode_binary(data + cur, 16);
    printf("| ");
    dump_decode_ascii(data + cur, 16);
    printf(" |");
    puts("");
  }
  /* write the remaining */
  if (len % 16) {
    dump_decode_binary(data + cur, len % 16);
    /* write the empty */
    for (size_t i = 0; i < 16 - (len % 16); i++) {
      printf("   ");
      if (i == 7 && (7 < 16 - (len % 16))) putchar(' ');
    }
    printf("| ");
    dump_decode_ascii(data + cur, len % 16);
    for (size_t i = 0; i < 16 - (len % 16); i++) {
      putchar(' ');
    }
    printf(" |");
    puts("");
    cur += len % 16;
  }
  assert(len == cur);
}


static ezgrpc2_header_t *header_create(const u8 *name, size_t nlen, const u8 *value, size_t vlen) {
  ezgrpc2_header_t *ezheader = malloc(sizeof(*ezheader));
  if (ezheader == NULL)
    return NULL;

  ezheader->name = malloc(nlen);
  ezheader->value = malloc(vlen);

  memcpy(ezheader->name, name, nlen);
  memcpy(ezheader->value, value, vlen);
  ezheader->nlen = nlen;
  ezheader->vlen = vlen;

  return ezheader;
}

static void header_free(ezgrpc2_header_t *ezheader) {
  free(ezheader->name);
  free(ezheader->value);
}



static i8 *ezgetdt() {
  static i8 buf[32] = {0};
  time_t t = time(NULL);
  struct tm stm = *localtime(&t);
  snprintf(buf, 32, COLSTR("%d-%02d-%02d %02d:%02d:%02d", BGRN),
           stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday, stm.tm_hour,
           stm.tm_min, stm.tm_sec);
  return buf;
}






static void ezlogf(ezgrpc2_session_t *ezsession, i8 *file, int line, i8 *fmt,
                   ...) {
  fprintf(logging_fp, "[%s @ %s:%d] " COLSTR("%s%s%s:%d", BHBLU) " ", ezgetdt(),
          file, line, (ezsession->sockaddr.ss_family == AF_INET6 ? "[" : ""),
          ezsession->client_addr, (ezsession->sockaddr.ss_family == AF_INET6 ? "]" : ""),
          ezsession->client_port);

  va_list args;
  va_start(args, fmt);
  vfprintf(logging_fp, fmt, args);
  va_end(args);
}

void ezlog(i8 *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(logging_fp, "[%s] ", ezgetdt());
  vfprintf(logging_fp, fmt, args);
  va_end(args);
}



/* returns a value where a whole valid message is found
 * f the return value equals vec.len, then the message
 * is complete and not truncated.
 */
static size_t count_grpc_message(void *data, size_t len, int *nb_message) {
  i8 *wire = (i8 *)data;
  size_t seek, last_seek = 0;
  for (seek = 0; seek < len; ) {
    /* compressed flag */
    seek += 1;

//    if (seek + 4 > len) {
//      atlog(
//          COLSTR(
//              "(1) prefixed-length messages overflowed. seeked %zu. len %zu\n",
//              BHYEL),
//          seek, len);
//      return last_seek;
//    }
    /* message length */
    u32 msg_len = ntohl(uread_u32(wire + seek));
    /* 4 bytes message lengtb */
    seek += sizeof(u32);

    seek += msg_len;

    if (seek <= len)
      last_seek = seek;
  }

  ezlog("lseek %zu len %zu\n", last_seek, len);

  if (seek > len) {
    atlog(COLSTR("(2) prefixed-length messages overflowed\n", BHYEL));
  }

  if (seek < len) {
    atlog(COLSTR("prefixed-length messages underflowed\n", BHYEL));
  }

  return last_seek;
}


/* constructs a linked list from a prefixed-length message. */
static size_t parse_grpc_message(void *data, size_t len, ezgrpc2_list_t *lmessages) {
  i8 *wire = data;

  size_t seek, last_seek = 0;
  for (seek = 0; seek < len;) {
    if (seek + 1 > len) {
      atlog("exceeded compressed flag\n");
      return last_seek;
    }
    i8 is_compressed = wire[seek];
    seek += 1;

    if (seek + 4 > len) {
      atlog("exceeded length prefixed\n");
      return last_seek;
    }
    u32 msg_len = ntohl(uread_u32(wire + seek));
    seek += 4;
    void *msg_data = wire + seek;

    if (seek + msg_len <= len) {
      last_seek = seek;
      ezgrpc2_message_t *msg = calloc(1, sizeof(ezgrpc2_message_t));
      assert(msg != NULL);

      msg->is_compressed = is_compressed;
      msg->len = msg_len;
      msg->data = malloc(msg->len);
      assert(msg->data != NULL);

      memcpy(msg->data, msg_data, msg_len);
      ezgrpc2_list_push_back(lmessages, msg);
    }
    seek += msg_len;
  }

 // atlog("ok\n");
  return last_seek;
}

static int ezgrpc2_list_cmp_ezstream_id(const void *data,const void *userdata) {
  return ((ezgrpc2_stream_t*)data)->stream_id != *(i32*)userdata;
}
static int ezgrpc2_list_cmp_ezheader_name(const void *data, const void *userdata) {
  const ezgrpc2_header_t *a = data;
  const ezgrpc2_header_t *b = userdata;
  return a->nlen == b->nlen ?
      strncasecmp(a->name, b->name,
        a->nlen & b->nlen): 1;
}

/* creates a ezstream and adds it to the linked list */

/* only frees what is passed. it does not free all the linked lists */
static void stream_free(ezgrpc2_stream_t *ezstream) {
  atlog("stream %d freed\n", ezstream->stream_id);
  ezgrpc2_header_t *ezheader;

  while ((ezheader = ezgrpc2_list_pop_front(ezstream->lheaders)) != NULL) {
    header_free(ezheader);
  }
  free(ezstream->recv_data);
  ezgrpc2_list_free(ezstream->lheaders);
  free(ezstream);
}

static nghttp2_ssize data_source_read_callback2(nghttp2_session *session,
                                         i32 stream_id, u8 *buf, size_t buf_len,
                                         u32 *data_flags,
                                         nghttp2_data_source *source,
                                         void *user_data) {
  if (buf_len < 5) {
    atlog(COLSTR("error buf_len < 5\n", BHRED));
    return NGHTTP2_INTERNAL_ERROR;
  }
//  ezgrpc2_stream_t *ezstream = nghttp2_session_get_stream_user_data(session, stream_id);
//  assert(ezstream != NULL);
  ezgrpc2_stream_t *ezstream = source->ptr;
  ezgrpc2_list_t *lmessages = ezstream->lqueue_omessages;
  ezgrpc2_message_t *msg;


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
  atlog("read done %d bytes\n", buf_seek);
 // *data_flags = NGHTTP2_DATA_FLAG_EOF | NGHTTP2_DATA_FLAG_NO_END_STREAM;
  return buf_seek;
}

#if 0
static nghttp2_ssize data_source_read_callback2(nghttp2_session *session,
                                         i32 stream_id, u8 *buf, size_t length,
                                         u32 *data_flags,
                                         nghttp2_data_source *source,
                                         void *user_data) {
  atlog("called\n");
  EZSSIZE s = read(source->fd, buf, length);
  atlog("read done\n");
  if (s < 0) {
#ifdef _WIN32
    if (WSAGetLastError() == WSAEWOULDBLOCK) {
#else
    if (errno == EWOULDBLOCK) {
#endif
      *data_flags = NGHTTP2_DATA_FLAG_EOF | NGHTTP2_DATA_FLAG_NO_END_STREAM;
      return 0;
    }
    atlog("pfd read err\n");
    perror("read");
    return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
  }
  else if (s == 0) {
    *data_flags = NGHTTP2_DATA_FLAG_EOF | NGHTTP2_DATA_FLAG_NO_END_STREAM;
  }
  atlog("read %zd bytes\n", s);
  return s;
  /*
  ezgrpc2_stream_t *ezstream = nghttp2_session_get_stream_user_data(session, stream_id);
  ezgrpc2_list_t *lresponse = source->ptr;
  if (lresponse == NULL) {
    atlog("source->ptr null\n");
    *data_flags = NGHTTP2_DATA_FLAG_EOF | NGHTTP2_DATA_FLAG_NO_END_STREAM;
    return 1;
  }

  ezgrpc2_response_t *ezresponse = ezgrpc2_list_peekb(lresponse);
  if (ezresponse == NULL) {
    *data_flags = NGHTTP2_DATA_FLAG_EOF | NGHTTP2_DATA_FLAG_NO_END_STREAM;
    return 0;
  }

  ezgrpc2_message_t *msg = ezgrpc2_list_popb(&ezresponse->lmessages);
  if (msg != NULL) {
    atlog("msg concat\n");
    buf[0] = msg->is_compressed;
    u32 len = htonl(msg->len);
    memcpy(buf + 1, &len, 4);
    memcpy(buf + 5, msg->data, msg->len);
    free(msg->data);
    *data_flags = NGHTTP2_DATA_FLAG_EOF | NGHTTP2_DATA_FLAG_NO_END_STREAM;
    return 5 + msg->len;
  }
  atlog("eof\n");
  free(ezgrpc2_list_popb(lresponse));
  return 0;
  */
}
#endif




/* ----------- BEGIN NGHTTP2 CALLBACKS ------------------*/

static nghttp2_ssize ngsend_callback(nghttp2_session *session, const u8 *data,
                               size_t length, int flags, void *user_data) {
  ezgrpc2_session_t *ezsession = user_data;

  EZSSIZE ret = send(ezsession->sockfd, data, length, 0);
  if (ret < 0) {
    atlog("error %s\n", strerror(errno));
    if (errno == EAGAIN || errno == EWOULDBLOCK) return NGHTTP2_ERR_WOULDBLOCK;

    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }

#ifdef EZENABLE_DEBUG
  atlog("ok. read %zu bytes\n", ret);
  //ezdump((void *)data, ret);
#endif
  return ret;
}








static nghttp2_ssize ngrecv_callback(nghttp2_session *session, u8 *buf, size_t length,
                               int flags, void *user_data) {
  ezgrpc2_session_t *ezsession = user_data;

  EZSSIZE ret = recv(ezsession->sockfd, buf, length, 0);
  if (ret == EZSOCKET_ERROR) {
#ifdef _WIN32
    int wsa;
    switch ((wsa = WSAGetLastError())) {
      case WSAEWOULDBLOCK:
      case WSAEINTR:
        return NGHTTP2_ERR_WOULDBLOCK;
      default:
        atlog("failure errno %d\n", wsa);
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
#else
    switch (errno) {
      case EWOULDBLOCK:
        return NGHTTP2_ERR_WOULDBLOCK;
      default:
        atlog("failure: %s\n", strerror(errno));
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
#endif
  }

#ifdef EZENABLE_DEBUG
  atlog("ok. read %zu bytes\n", ret);
#endif
  return ret;
}







/* we're beginning to receive a header; open up a memory for the new stream */
static int on_begin_headers_callback(nghttp2_session *session,
                                     const nghttp2_frame *frame,
                                     void *user_data) {

  ezgrpc2_session_t *ezsession = user_data;
  ezgrpc2_stream_t *ezstream = calloc(1, sizeof(*ezstream));
  if (ezstream == NULL) {
#ifdef EZENABLE_DEBUG
    atlog("stream id %d doesn't exist \n", frame->hd.stream_id);
#endif
    return NGHTTP2_ERR_CALLBACK_FAILURE;
  }

  ezstream->lqueue_omessages = ezgrpc2_list_new(NULL);
  ezstream->lheaders = ezgrpc2_list_new(NULL);

  ezstream->time = (uint64_t)time(NULL);
  ezstream->stream_id = frame->hd.stream_id;
  ezstream->recv_len = 0;
  ezstream->recv_data = malloc(EZWINDOW_SIZE);

  ezstream->is_trunc = 0;
  ezstream->trunc_seek = 0;
  ezgrpc2_list_push_back(ezsession->lstreams, ezstream);
  nghttp2_session_set_stream_user_data(session, frame->hd.stream_id ,ezstream);
  //ezsession->nb_open_streams++;

#ifdef EZENABLE_DEBUG
  atlog("stream id %d \n",
         frame->hd.stream_id);
#endif
  return 0;
}









/* ok, here's the header name and value. this will be called from time to time
 */
static int on_header_callback(nghttp2_session *session,
                              const nghttp2_frame *frame, const u8 *name,
                              size_t namelen, const u8 *value, size_t valuelen,
                              u8 flags, void *user_data) {

  (void)flags;
  ezgrpc2_session_t *ezsession = user_data;
  ezgrpc2_stream_t *ezstream = nghttp2_session_get_stream_user_data(ezsession->ngsession, frame->hd.stream_id);

  ezgrpc2_header_t *ezheader = header_create(name, namelen, value, valuelen);
  assert(ezheader != NULL);
  ezgrpc2_list_push_back(ezstream->lheaders, ezheader);

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
    ezgrpc2_stream_t *ezstream = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
    if (ezstream == NULL) {
      atlog("couldn't find stream %d\n", frame->hd.stream_id);
      return 1;
    }
    for (size_t i = 0; i < ezsession->nb_paths; i++) 
      if (!EZSTRCMP(ezsession->paths[i].path, value)) {
        ezstream->path = &ezsession->paths[i];
  #ifdef EZENABLE_DEBUG
        atlog("found service %s\n", ezsession->paths[i].path);
  #endif
        break;
      }
    if (ezstream->path == NULL)
      atlog("path not found %s\n", value);
  }
  else if (!EZSTRCMP("content-type", name)) {
    int is_gt = valuelen >= 16;
    ezstream->is_content_grpc = is_gt ? !strncmp("application/grpc", (void *)value, 16) : 0;
  }
  else if (!EZSTRCMP("te", name)) {
    ezstream->is_te_trailers = !EZSTRCMP("trailers", value);
  }
  else {
    ;
  }
#undef EZSTRCMP
  /* clang-format on */

#ifdef EZENABLE_DEBUG
  atlog("header type: %d, %s: %.*s\n", frame->hd.type, name, (int)valuelen,
         value);
#endif

  return 0;
}










static inline int on_frame_recv_headers(ezgrpc2_session_t *ezsession, const nghttp2_frame *frame) {
  if (!(frame->hd.flags & NGHTTP2_FLAG_END_HEADERS)) return 0;
  /* if we've received an end stream in headers frame. send an RST. we
   * expected a data */
#if 0
  if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) {
    nghttp2_submit_rst_stream(ezsession->ngsession, NGHTTP2_FLAG_NONE,
                              frame->hd.stream_id, 1);
    return 0;
  }
#endif
 
  ezgrpc2_stream_t *ezstream = nghttp2_session_get_stream_user_data(ezsession->ngsession, frame->hd.stream_id);
     // (void*)&frame->hd.stream_id);
  if (ezstream == NULL) {
    atlog("stream %d not found. sending rst\n", frame->hd.stream_id);
    nghttp2_submit_rst_stream(ezsession->ngsession, NGHTTP2_FLAG_NONE,
                              frame->hd.stream_id,
                              1);  // XXX send appropriate code
    return 0;
  }
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
    atlog("received a header with an end stream. sending rst\n", frame->hd.stream_id);
    nghttp2_submit_rst_stream(ezsession->ngsession, NGHTTP2_FLAG_NONE,
                              frame->hd.stream_id, 1);
    return 0;
  }

  if (!(ezstream->is_method_post && ezstream->is_scheme_http &&
        ezstream->is_te_trailers && ezstream->is_content_grpc)) {
    atlog("not method_post %d && is_scheme_http %d && is_te_trailers %d && is_content_grpc %d. sending rst\n", ezstream->is_method_post, ezstream->is_scheme_http, ezstream->is_te_trailers, ezstream->is_content_grpc);
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
                           NULL)) {assert(0);}
  if (ezstream->path == NULL) {
    atlog("path null, submitting req\n");
    int status = EZGRPC2_STATUS_UNIMPLEMENTED;
    char grpc_status[32] = {0};
    int len = snprintf(grpc_status, 31, "%d",(int) status);
    i8 *grpc_message = ezgrpc2_status_strstatus(status);
    nghttp2_nv trailers[] = {
        {(void *)"grpc-status", (void *)grpc_status, 11, len},
        {(void *)"grpc-message", (void *)grpc_message, 12,
         strlen(grpc_message)},
    };
    nghttp2_submit_trailer(ezsession->ngsession, frame->hd.stream_id, trailers, 2);
    atlog("trailer submitted\n");
   // nghttp2_session_send(ezsession->ngsession);
   // ezgrpc2_session_end_stream(ezsession, ezstream->stream_id, EZGRPC2_STATUS_UNIMPLEMENTED);
    return 0;
  }
  atlog("ret id %d\n", frame->hd.stream_id);
  return 0;
}








static inline int on_frame_recv_settings(ezgrpc2_session_t *ezsession, const nghttp2_frame *frame) {
  if (frame->hd.stream_id != 0){
    atlog("received a SETTINGS frame at stream_id 0. immediately returning\n");
    return 0;
  }

  atlog("ack %d, length %zu\n", frame->settings.hd.flags,
         frame->settings.hd.length);
  if (!(frame->settings.hd.flags & NGHTTP2_FLAG_ACK)) {
    // apply settings
    for (size_t i = 0; i < frame->settings.niv; i++) {
      switch (frame->settings.iv[i].settings_id) {
        case NGHTTP2_SETTINGS_HEADER_TABLE_SIZE:
          ezsession->csettings.header_table_size =
              frame->settings.iv[i].value;
          break;
        case NGHTTP2_SETTINGS_ENABLE_PUSH:
          ezsession->csettings.enable_push = frame->settings.iv[i].value;
          break;
        case NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS:
          ezsession->csettings.max_concurrent_streams =
              frame->settings.iv[i].value;
          break;
        case NGHTTP2_SETTINGS_MAX_FRAME_SIZE:
          ezsession->csettings.max_frame_size = frame->settings.iv[i].value;
          // case NGHTTP2_SETTINGS_FLOW_CONTROL:
          // ezstream->flow_control =  frame->settings.iv[i].value;
          break;
        default:
          break;
      };
    }
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










static inline int on_frame_recv_data(ezgrpc2_session_t *ezsession, const nghttp2_frame *frame) {

  ezgrpc2_stream_t *ezstream = nghttp2_session_get_stream_user_data(ezsession->ngsession, frame->hd.stream_id);
  if (ezstream == NULL) {
    atlog("stream %d not found. sending rst\n", frame->hd.stream_id);
    nghttp2_submit_rst_stream(ezsession->ngsession, NGHTTP2_FLAG_NONE,
                              frame->hd.stream_id, 1);
    return 0;
  }
  if (ezstream->path == NULL)
    return 0;

  ezgrpc2_list_t *lmessages = ezgrpc2_list_new(NULL);

  if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM && ezstream->recv_len == 0) {
    /* In scenarios where the Request stream needs to be closed but no data remains
     * to be sent implementations MUST send an empty DATA frame with this flag set. */
    atlog("event message %zu\n", ezgrpc2_list_count(lmessages));
    ezgrpc2_event_t *event = ezgrpc2_event_new(
      EZGRPC2_EVENT_MESSAGE,
      ezgrpc2_session_uuid_copy(ezsession->session_uuid),
      ((ezgrpc2_event_message_t) {
        .lmessages = lmessages,
        .end_stream = 1,
        .stream_id = frame->hd.stream_id,
      })
    );

    ezgrpc2_list_push_back(ezstream->path->levents, event);
    return 0;
  }

  size_t lseek = parse_grpc_message(ezstream->recv_data, ezstream->recv_len, lmessages);
  if (lseek != 0) {
    assert(lseek <= ezstream->recv_len);
    atlog("event message %zu\n", ezgrpc2_list_count(lmessages));
    memcpy(ezstream->recv_data, ezstream->recv_data + lseek, ezstream->recv_len - lseek);
    ezstream->recv_len -= lseek;
    ezgrpc2_event_t *event = ezgrpc2_event_new(
      EZGRPC2_EVENT_MESSAGE,
      ezgrpc2_session_uuid_copy(ezsession->session_uuid),
      ((ezgrpc2_event_message_t) {
        .lmessages = lmessages,
        .end_stream = frame->hd.flags & NGHTTP2_FLAG_END_STREAM && ezstream->recv_len == 0,
        .stream_id = frame->hd.stream_id,
      })
    );

    ezgrpc2_list_push_back(ezstream->path->levents, event);
  }
  if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM && ezstream->recv_len != 0) {
    /* we've receaived an end stream but last message is truncated */
    atlog("event dataloss %zu\n", ezgrpc2_list_count(lmessages));
    ezgrpc2_event_t *event = ezgrpc2_event_new(
      EZGRPC2_EVENT_DATALOSS,
      ezgrpc2_session_uuid_copy(ezsession->session_uuid),
      ((ezgrpc2_event_dataloss_t) {
        .stream_id = ezstream->stream_id,
      })
    );
  
    ezgrpc2_list_push_back(ezstream->path->levents, event);
    //event->message.end_stream = !!(frame->hd.flags & NGHTTP2_FLAG_END_STREAM);
  }
  return 0;
}













static int on_frame_recv_callback(nghttp2_session *session,
                                  const nghttp2_frame *frame, void *user_data) {
#ifdef EZENABLE_DEBUG
  atlog("frame type: %d, stream id %d\n", frame->hd.type, frame->hd.stream_id);
#endif
  ezgrpc2_session_t *ezsession = user_data;
  switch (frame->hd.type) {
    case NGHTTP2_SETTINGS:
      return on_frame_recv_settings(ezsession, frame);
    case NGHTTP2_HEADERS:
      return on_frame_recv_headers(ezsession, frame);
    case NGHTTP2_DATA:
      return on_frame_recv_data(ezsession, frame);
    case NGHTTP2_WINDOW_UPDATE:
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
  ezgrpc2_session_t *ezsession = user_data;
  ezgrpc2_stream_t *ezstream = nghttp2_session_get_stream_user_data(session, stream_id);
  //ezgrpc2_stream_t *ezstream = ezgrpc2_list_find(&ezsession->lstreams, ezgrpc2_list_cmp_ezstream_id, &stream_id);
  if (ezstream == NULL) {
    atlog("stream_id %d doesn't exist\n", stream_id);
    return 0;
  }


  /* prevent an attacker from pushing large POST data */
  if ((ezstream->recv_len + len) > EZWINDOW_SIZE) {
    nghttp2_submit_rst_stream(ezsession->ngsession, NGHTTP2_FLAG_NONE,
                              stream_id, 1);  // XXX send appropriate code
    atlog(COLSTR("POST data exceeds maximum allowed size, killing stream %d\n",
                  BHRED),
           stream_id);
    return 1;
  }


  memcpy(ezstream->recv_data + ezstream->recv_len, data, len);
  ezstream->recv_len += len;
#ifdef EZENABLE_DEBUG
  atlog("data chunk recv %zu bytes at stream %d\n", len, stream_id);
#endif

  return 0;
}

int on_stream_close_callback(nghttp2_session *session, i32 stream_id,
                             u32 error_code, void *user_data) {
  atlog("stream %d closed\n", stream_id);
  ezgrpc2_session_t *ezsession = user_data;
  ezgrpc2_stream_t *ezstream = nghttp2_session_get_stream_user_data(session, stream_id);
  assert(ezstream == ezgrpc2_list_remove(ezsession->lstreams, ezgrpc2_list_cmp_ezstream_id, &stream_id));
  stream_free(ezstream);

  return 0;
}


/* ----------- END NGHTTP2 CALLBACKS ------------------*/











static int makenonblock(int sockfd) {
#ifdef _WIN32
  return ioctlsocket(sockfd, FIONBIO, &(u_long){1});
#else
  int res = fcntl(sockfd, F_GETFL, 0);
  if (res == -1) return 1;
  res = (res | O_NONBLOCK);
  return fcntl(sockfd, F_SETFL, res);
#endif

}








static int server_setup_ngcallbacks(nghttp2_session_callbacks *ngcallbacks) {
  /* clang-format off */
  nghttp2_session_callbacks_set_send_callback(ngcallbacks, ngsend_callback);
  nghttp2_session_callbacks_set_recv_callback(ngcallbacks, ngrecv_callback);

  /* prepares and allocates memory for the incoming HEADERS frame */
  nghttp2_session_callbacks_set_on_begin_headers_callback(ngcallbacks, on_begin_headers_callback);
  /* we received that HEADERS frame. now here's the http2 fields */
  nghttp2_session_callbacks_set_on_header_callback(ngcallbacks, on_header_callback);
  /* we received a frame. do something about it */
  nghttp2_session_callbacks_set_on_frame_recv_callback(ngcallbacks, on_frame_recv_callback);
//  nghttp2_session_callbacks_set_on_frame_send_callback(ngcallbacks, on_frame_send_callback);

  //  nghttp2_session_callbacks_set_on_frame_send_callback(callbacks,
  //                                                       on_frame_send_callback);
  nghttp2_session_callbacks_set_on_data_chunk_recv_callback(ngcallbacks, on_data_chunk_recv_callback);
  nghttp2_session_callbacks_set_on_stream_close_callback(ngcallbacks, on_stream_close_callback);
 // nghttp2_session_callbacks_set_send_data_callback(ngcallbacks, send_data_callback);
  //  nghttp2_session_callbacks_set_before_frame_send_callback(callbacks,
  //  before_frame_send_callback);

  /* clang-format on */

  return 0;
}








static EZNFDS get_unused_pollfd_ndx(struct pollfd *fds, EZNFDS nb_fds) {
  for (EZNFDS i = 2; i < nb_fds; i++)
    if (fds[i].fd == -1)
      return i;
  return -1;
}








static void session_free(ezgrpc2_session_t *ezsession) {
  nghttp2_session_terminate_session(ezsession->ngsession, 0);
  nghttp2_session_del(ezsession->ngsession);

  ezgrpc2_stream_t *ezstream;
  while ((ezstream = ezgrpc2_list_pop_front(ezsession->lstreams)) != NULL)
    stream_free(ezstream);

  ezgrpc2_session_uuid_free(ezsession->session_uuid);
  ezsession->session_uuid = NULL;

  memset(ezsession, 0, sizeof(*ezsession));
}








static ezgrpc2_session_t *session_find(ezgrpc2_session_t *ezsessions, size_t nb_ezsessions, ezgrpc2_session_uuid_t *session_uuid) {
  for (size_t i = 0; i < nb_ezsessions; i++)
    if (ezgrpc2_session_uuid_is_equal(ezsessions[i].session_uuid, session_uuid))
      return ezsessions + i;

  return NULL;
}















static int session_create(
    ezgrpc2_session_t *ezsession,
    EZSOCKET sockfd,
    struct sockaddr_storage *sockaddr,
    EZSOCKLEN socklen,
    ezgrpc2_server_t *server) {

  /* PREPARE NGHTTP2 */
  nghttp2_session_callbacks *ngcallbacks;
  int res = nghttp2_session_callbacks_new(&ngcallbacks);
  if (res) {
    shutdown(sockfd, SHUT_RDWR);
    assert(0);  // TODO
  }


  /**********************.
  |     INIT NGHTTP2     |
  `---------------------*/
  server_setup_ngcallbacks(ngcallbacks);

  res =
      nghttp2_session_server_new(&ezsession->ngsession, ngcallbacks, ezsession);
  nghttp2_session_callbacks_del(ngcallbacks);
  if (res < 0) {
    assert(0);  // TODO
  }

  /* 16kb frame size */
  nghttp2_settings_entry siv[] = {
      {NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, EZWINDOW_SIZE},
      {NGHTTP2_SETTINGS_MAX_FRAME_SIZE, 16 * 1024},
      {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 32},
  };
  res =
      nghttp2_submit_settings(ezsession->ngsession, NGHTTP2_FLAG_NONE, siv, 2);
  if (res < 0) {
    assert(0);  // TODO
  }

  if (makenonblock(sockfd)) assert(0);

  res = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (int *)&(int){1},
                   sizeof(int));
  if (res < 0) {
    assert(0);  // TODO
  }

  res = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (int *)&(int){1},
                   sizeof(int));
  if (res < 0) {
    assert(0);  // TODO
  }

  //ezsession->shutdownfd = server_handle->shutdownfd;
  ezsession->sockfd = sockfd;
  ezsession->sockaddr = *sockaddr;
  ezsession->socklen = socklen;
  switch (sockaddr->ss_family) {
    case AF_INET: {
      struct sockaddr_in *sa = (struct sockaddr_in *)sockaddr;
      inet_ntop(AF_INET, &sa->sin_addr.s_addr,
                ezsession->client_addr, sizeof(ezsession->client_addr) - 1);
      ezsession->client_port = sa->sin_port;

//      ezsession->server_addr = server_handle->ipv4_addr;
//      ezsession->server_port = &server_handle->ipv4_port;
                  } break;
    case AF_INET6: {
      struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)sockaddr;
      inet_ntop(AF_INET6, sa6->sin6_addr.s6_addr,
                ezsession->client_addr, sizeof(ezsession->client_addr) - 1);
      ezsession->client_port = sa6->sin6_port;

//      ezsession->server_addr = server_handle->ipv6_addr;
//      ezsession->server_port = &server_handle->ipv6_port;
                   } break;
    default:
      fprintf(stderr, "fatal: invalid family\n");
      return 1;
  }
  /*  all seems to be successful. set the following */
  ezsession->lstreams = ezgrpc2_list_new(NULL);
  ezsession->session_uuid = ezgrpc2_session_uuid_new(NULL);
  //ezsession->alive = 1;

  return res;
}


static int session_add(ezgrpc2_server_t *ezserver, int listenfd) {
  struct sockaddr_storage sockaddr;
  struct pollfd *fds = ezserver->fds;
  EZNFDS nb_fds = ezserver->nb_fds;
  EZSOCKLEN sockaddr_len;


  sockaddr_len = sizeof(sockaddr);
  memset(&sockaddr, 0, sizeof(sockaddr));
  int confd = accept(listenfd, (struct sockaddr *)&sockaddr, &sockaddr_len);
  if (confd == -1) {
    perror("accept");
    return 1;
  }
  ezlog(COLSTR("incoming %s connection\n", BHBLU), sockaddr.ss_family == AF_INET ? "ipv4" : "ipv6");

  EZNFDS ndx = get_unused_pollfd_ndx(fds, nb_fds);
  if (ndx == -1) {
    ezlog("max clients reached\n");
    shutdown(confd, SHUT_RDWR);
    close(confd);
    return 1;
  }

  if (session_create(&ezserver->sessions[ndx], confd, &sockaddr, sockaddr_len, ezserver)) {
    atlog("session create failed\n");
    shutdown(confd, SHUT_RDWR);
    close(confd);
    return 1;
  }
  fds[ndx].fd = confd;
  fds[ndx].events = POLLIN | POLLRDHUP;
  return 0;
}





static int session_events(ezgrpc2_session_t *ezsession) {
  int res;
  if (!nghttp2_session_want_read(ezsession->ngsession) &&
      !nghttp2_session_want_write(ezsession->ngsession))
    return 1;

  if (nghttp2_session_want_read(ezsession->ngsession) &&
      (res = nghttp2_session_recv(ezsession->ngsession)))
    if (res) {
      ezlog(COLSTR("nghttp2: %s. killing...\n", BHRED),
            nghttp2_strerror(res));
      return 1;
    }

  while (nghttp2_session_want_write(ezsession->ngsession)) {
    res = nghttp2_session_send(ezsession->ngsession);
    if (res) {
      ezlog(COLSTR("nghttp2: %s. killing...\n", BHRED),
            nghttp2_strerror(res));
      return 1;
    }
  }
  return 0;
}
/* clang-format off */













/* THIS IS INTENTIONALLY LEFT BLANK */














/*------------------------------------------------------.
| API FUNCTIONS: You will only have to care about these |
`------------------------------------------------------*/

void ezgrpc2_server_free(
  ezgrpc2_server_t *s) {
  for (EZNFDS i = 0; i < 2; i++)
  {
    if (s->fds[i].fd != -1) {
      close(s->fds[i].fd);
    }
  }

  for (EZNFDS i = 2; i < s->nb_fds; i++)
  {
    if (s->fds[i].fd != -1) {
      shutdown(s->fds[i].fd, SHUT_RDWR);
      close(s->fds[i].fd);
      session_free(&s->sessions[i]);
    }
  }
  free(s->fds);
    
  free(s->ipv4_addr);
  free(s->ipv6_addr);
  free(s);
}










ezgrpc2_server_t *ezgrpc2_server_new(
  const char *ipv4_addr, u16 ipv4_port,
  const char *ipv6_addr, u16 ipv6_port,
  int backlog,
  void *settings) {
  (void)settings;
  struct sockaddr_in ipv4_saddr = {0};
  struct sockaddr_in6 ipv6_saddr = {0};
  ezgrpc2_server_t *server = NULL;
#ifdef _WIN32
  int ret;
  WSADATA wsa_data = {0};
  if ((ret = WSAStartup(0x0202, &wsa_data))) {
    ezlog("WSAStartup failed: error %d\n", ret);
    return NULL;
  }
#endif
  EZSOCKET ipv4_sockfd = EZINVALID_SOCKET;
  EZSOCKET ipv6_sockfd = EZINVALID_SOCKET;

  if (ipv4_addr == NULL && ipv6_addr == NULL)
    return NULL;
  if (ipv4_addr != NULL && strnlen(ipv4_addr, 16) > 15)
    return NULL;
  if (ipv6_addr != NULL && strnlen(ipv6_addr, 64) > 63)
    return NULL;

  server = calloc(1, sizeof(*server));
  assert(server != NULL);

  server->ipv4_addr = NULL;
  server->ipv6_addr = NULL;
  server->ipv4_port = ipv4_port;
  server->ipv6_port = ipv6_port;
  if (ipv4_addr != NULL) {
    server->ipv4_addr = strndup(ipv4_addr, 16);
    assert(server->ipv4_addr != NULL);
  }
  if (ipv6_addr != NULL) {
    server->ipv6_addr = strndup(ipv6_addr, 64);
    assert(server->ipv6_addr != NULL);
  }

  /* shutdown fd + ipv4 fd listener + ipv6 fd listener */
  /**********************.
  |     INIT SOCKET      |
  `---------------------*/

#ifdef _WIN32
#define perror(s) fprintf(stderr, "WSA: " s " error code: %d\n", WSAGetLastError())
#endif

  if (server->ipv4_addr != NULL) {
    memset(&ipv4_saddr, 0, sizeof(ipv4_saddr));
    ipv4_saddr.sin_family = AF_INET;
    ipv4_saddr.sin_addr.s_addr = inet_addr(server->ipv4_addr);
    ipv4_saddr.sin_port = htons(server->ipv4_port);

    if ((ipv4_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == EZINVALID_SOCKET) {
      perror("socket");
      goto err;
    }

    if (setsockopt(ipv4_sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1},
                   sizeof(int))) {
      perror("setsockopt ipv4");
      goto err;
    }

    if (bind(ipv4_sockfd, (struct sockaddr *)&ipv4_saddr, sizeof(ipv4_saddr)) == EZSOCKET_ERROR) {
      perror("bind ipv4");
      goto err;
    }

    if (listen(ipv4_sockfd, backlog) == EZSOCKET_ERROR) {
      perror("listen ipv4");
      goto err;
    }

    if (makenonblock(ipv4_sockfd)) assert(0);  // TODO

    ezlog("listening on ipv4 %s:%d ...\n", server->ipv4_addr,
          server->ipv4_port);
  }

  if (server->ipv6_addr != NULL) {
    memset(&ipv6_saddr, 0, sizeof(ipv6_saddr));
    ipv6_saddr.sin6_family = AF_INET6;
    if (inet_pton(AF_INET6, server->ipv6_addr,
                  ipv6_saddr.sin6_addr.s6_addr) != 1) {
      perror("inet_pton");
      goto err;
    }
    ipv6_saddr.sin6_port = htons(server->ipv6_port);

    if ((ipv6_sockfd = socket(AF_INET6, SOCK_STREAM, 0)) == EZINVALID_SOCKET) {
      perror("socket(inet6,sockstream,0)");
      goto err;
    }

    if (setsockopt(ipv6_sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&(int){1},
                   sizeof(int))) {
      perror("setsockopt ipv6");
      goto err;
    }
    if (setsockopt(ipv6_sockfd, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&(int){1},
                   sizeof(int))) {
      perror("setsockopt ipv6");
      goto err;
    }

    if (bind(ipv6_sockfd, (struct sockaddr *)&ipv6_saddr, sizeof(ipv6_saddr)) == EZSOCKET_ERROR) {
      perror("bind ipv6");
      goto err;
    }

    if (listen(ipv6_sockfd, 16) == EZSOCKET_ERROR) {
      perror("listen ipv6");
      goto err;
    }

    if (makenonblock(ipv6_sockfd)) assert(0);  // TODO

    ezlog("listening on ipv6 [%s]:%d ...\n", server->ipv6_addr,
          server->ipv6_port);
  }

#define MAX_CON_CLIENTS 1022
  server->nb_sessions = MAX_CON_CLIENTS + 2;
  server->sessions = calloc(MAX_CON_CLIENTS + 2, sizeof(*server->sessions));

  server->nb_fds = MAX_CON_CLIENTS + 2;
  server->fds = calloc(server->nb_fds, sizeof(*(server->fds)));
  if (server->fds == NULL)
    goto err;
  server->fds[0].fd = ipv4_sockfd;
  server->fds[0].events = POLLIN | POLLRDHUP;
  server->fds[1].fd = ipv6_sockfd;
  server->fds[1].events = POLLIN | POLLRDHUP;

  for (EZNFDS i = 2; i < server->nb_fds; i++)
  {
    server->fds[i].fd = -1;
  }


#ifdef _WIN32
  WSACleanup();
#endif

  /* start accepting connections */

  return server;

err:
  ezgrpc2_server_free(server);
  return NULL;
}










int ezgrpc2_server_poll(
  ezgrpc2_server_t *server,
  ezgrpc2_path_t *paths,
  size_t nb_paths,
  int timeout) {
  struct pollfd *fds = server->fds;
  const EZNFDS nb_fds = server->nb_fds;

  const int ready = EZPOLL(fds, nb_fds, timeout);
  if (ready <= 0)
    return ready;

    // add ipv4 session
  if (fds[0].revents & POLLIN)
    session_add(server, fds[0].fd);
  // add ipv6 session
  if (fds[1].revents & POLLIN)
    session_add(server, fds[1].fd);

  for (EZNFDS i = 2; i < nb_fds; i++) {
    if (fds[i].fd != -1 && fds[i].revents & (POLLRDHUP | POLLERR)) {
      ezlog("c hangup\n");
      session_free(&server->sessions[i]);
      fds[i].fd = -1;
      fds[i].revents = 0;
      //sleep(30);
      continue;
    }
    if (fds[i].fd != -1 && fds[i].revents & POLLIN) {
      server->sessions[i].nb_paths = nb_paths;
      server->sessions[i].paths = paths;
      if (session_events(&server->sessions[i])) {
        if (close(server->sessions[i].sockfd)) {
          perror("close");
        }
        fds[i].fd = -1;
        session_free(&server->sessions[i]);
      }
      fds[i].revents = 0;
    }
  }

  return ready;
}









int ezgrpc2_session_send(
  ezgrpc2_server_t *ezserver,
  ezgrpc2_session_uuid_t *session_uuid,
  i32 stream_id,
  ezgrpc2_list_t *lmessages) {
  ezgrpc2_session_t *ezsession = session_find(ezserver->sessions, ezserver->nb_sessions, session_uuid);
  if (ezsession == NULL) {
    return 1;
  }
  ezgrpc2_stream_t *ezstream = nghttp2_session_get_stream_user_data(ezsession->ngsession, stream_id);
  if (ezstream == NULL) {
    /* stream doesn't exists */
    return 2;
  }

  ezgrpc2_list_concat_and_empty_src(ezstream->lqueue_omessages, lmessages);


  nghttp2_data_provider2 data_provider;
  data_provider.source.ptr = ezstream;
  data_provider.read_callback = data_source_read_callback2;
  nghttp2_submit_data2(ezsession->ngsession, 0, stream_id, &data_provider);
  nghttp2_session_send(ezsession->ngsession);
  return 0;
}











int ezgrpc2_session_end_stream(
  ezgrpc2_server_t *ezserver,
  ezgrpc2_session_uuid_t *session_uuid,
  i32 stream_id,
  int status) {
  ezgrpc2_session_t *ezsession = session_find(ezserver->sessions, ezserver->nb_sessions, session_uuid);
  if (ezsession == NULL) {
    return 1;
  }

  ezgrpc2_stream_t *ezstream = nghttp2_session_get_stream_user_data(ezsession->ngsession, stream_id);
  if (ezstream == NULL)
    return 2;

  char grpc_status[32] = {0};
  int len = snprintf(grpc_status, 31, "%d",(int) status);
  i8 *grpc_message = ezgrpc2_status_strstatus(status);
  nghttp2_nv trailers[] = {
      {(void *)"grpc-status", (void *)grpc_status, 11, len},
      {(void *)"grpc-message", (void *)grpc_message, 12,
       strlen(grpc_message)},
  };
  if (!ezgrpc2_list_is_empty(ezstream->lqueue_omessages)) {
    atlog(COLSTR("ending stream but messages are still queued\n", BHRED));

  }
  nghttp2_submit_trailer(ezsession->ngsession, stream_id, trailers, 2);
  atlog("trailer submitted\n");
  nghttp2_session_send(ezsession->ngsession);
  return 0;
}










int ezgrpc2_session_end_session(
  ezgrpc2_server_t *ezserver,
  ezgrpc2_session_uuid_t *session_uuid,
  i32 last_stream_id,
  int error_code) {
  ezgrpc2_session_t *ezsession = session_find(ezserver->sessions, ezserver->nb_sessions, session_uuid);
  if (ezsession == NULL) {
    /* session doesn't exists */
    return 1;
  }
  nghttp2_submit_goaway(ezsession->ngsession, 0, last_stream_id, error_code, NULL, 0);
  nghttp2_session_send(ezsession->ngsession);

  //shutdown(ezsession->sockfd, SHUT_RDWR);
  //memset(ezsession->session_id, 0, 32);
  return 0;
}

int ezgrpc2_session_find_header(
  ezgrpc2_server_t *ezserver,
  ezgrpc2_session_uuid_t *session_uuid,
  i32 stream_id, ezgrpc2_header_t *ezheader) {
  ezgrpc2_session_t *ezsession = session_find(ezserver->sessions, ezserver->nb_sessions, session_uuid);
  if (ezsession == NULL) {
    return 1;
  }

  ezgrpc2_stream_t *ezstream = nghttp2_session_get_stream_user_data(ezsession->ngsession, stream_id);
  if (ezstream == NULL)
    return 2;

  ezgrpc2_header_t *found;
  if ((found = ezgrpc2_list_find(ezstream->lheaders, ezgrpc2_list_cmp_ezheader_name, ezheader))
      == NULL)
    return 3;


  return ezheader->value = found->value, ezheader->vlen = found->vlen, 0;
}
  
