#include <assert.h>
#include <errno.h>
#include <time.h>
#include <stddef.h>
#include "core.h"
#include "ezgrpc2_header.h"
#include "internal_nghttp2_callbacks.h"
#include "internal_helpers.h"
#define atlog(...) (void)0
//#define ezlog(...) (void)0

extern ezgrpc2_event *event_new(ezgrpc2_event_type type, ezgrpc2_session_uuid *session_uuid, ...);

#ifdef _WIN32
int makenonblock(SOCKET sockfd) {
  return ioctlsocket(sockfd, FIONBIO, &(u_long){1});
}
#else
int makenonblock(int sockfd) {
  int res = fcntl(sockfd, F_GETFL, 0);
  if (res == -1) return 1;
  res = (res | O_NONBLOCK);
  return fcntl(sockfd, F_SETFL, res);
}
#endif

int list_cmp_ezheader_name(const void *data, const void *userdata) {
  const ezgrpc2_header *a = data;
  const ezgrpc2_header *b = userdata;
  return a->namelen == b->namelen ?
      strncasecmp(a->name, b->name,
        a->namelen & b->namelen): 1;
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

char *ezgetdt(char *buf, size_t len) {
#ifdef __unix__
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  char tmp[32] = {0};
  strftime(tmp, sizeof(tmp), "%F %T", localtime(&ts.tv_sec));
  snprintf(buf, len, COLSTR("%s.%09ld", BGRN), tmp, ts.tv_nsec);
#else
  time_t t = time(NULL);
  struct tm stm = *localtime(&t);
  snprintf(buf, len, COLSTR("%d/%02d/%02d %02d:%02d:%02d", BGRN),
           stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday, stm.tm_hour,
           stm.tm_min, stm.tm_sec);
#endif
  return buf;
}






//void ezlogf(ezgrpc2_session *ezsession, i8 *file, int line, i8 *fmt,
//                   ...) {
//  fprintf(logging_fp, "[%s @ %s:%d] " COLSTR("%s%s%s:%d", BHBLU) " ", ezgetdt(),
//          file, line, (ezsession->sockaddr.ss_family == AF_INET6 ? "[" : ""),
//          ezsession->client_addr, (ezsession->sockaddr.ss_family == AF_INET6 ? "]" : ""),
//          ezsession->client_port);
//
//  va_list args;
//  va_start(args, fmt);
//  vfprintf(logging_fp, fmt, args);
//  va_end(args);
//}

void ezlog(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char dt[128] = {0};
  fprintf(stderr, "[%s] ", ezgetdt(dt, sizeof(dt)));
  vfprintf(stderr, fmt, args);
  va_end(args);
}

void ezflog(FILE *fp, char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char dt[128] = {0};
  fprintf(fp, "[%s] ", ezgetdt(dt, sizeof(dt)));
  vfprintf(fp, fmt, args);
  va_end(args);
}

EZNFDS get_unused_pollfd_ndx(struct pollfd *fds, EZNFDS nb_fds) {
  for (EZNFDS i = 2; i < nb_fds; i++)
    if (fds[i].fd == -1)
      return i;
  return -1;
}








void session_free(ezgrpc2_session *ezsession) {
  ezgrpc2_event *event = event_new(EZGRPC2_EVENT_DISCONNECT, 
      ezgrpc2_session_uuid_copy(&ezsession->session_uuid));
  ezgrpc2_list_push_back(ezsession->server->levents, event);

  // Do not free this. It's not malloced
  // ezgrpc2_session_uuid_free(ezsession->session_uuid);

  nghttp2_session_terminate_session(ezsession->ngsession, 0);
  nghttp2_session_del(ezsession->ngsession);

  ezgrpc2_stream *ezstream;
  while ((ezstream = ezgrpc2_list_pop_front(ezsession->lstreams)) != NULL)
    stream_free(ezstream);

  //ezsession->session_uuid = NULL;

  memset(ezsession, 0, sizeof(*ezsession));
}








ezgrpc2_session *session_find(ezgrpc2_session *sessions, size_t nb_sessions, ezgrpc2_session_uuid *session_uuid) {
  assert(session_uuid->index < nb_sessions);
  return ezgrpc2_session_uuid_is_equal(session_uuid, &(sessions + session_uuid->index)->session_uuid) ? sessions + session_uuid->index: NULL;

}















int session_create(
    ezgrpc2_session *ezsession,
    EZSOCKET sockfd,
    struct sockaddr_storage *sockaddr,
    EZSOCKLEN socklen,
    ezgrpc2_server *server) {

  /* PREPARE NGHTTP2 */
  nghttp2_session_callbacks *ngcallbacks;
  int res = nghttp2_session_callbacks_new(&ngcallbacks);
  if (res) {
    return 1;
  }
  ezsession->server = server;


  /**********************.
  |     INIT NGHTTP2     |
  `---------------------*/
  server_setup_ngcallbacks(ngcallbacks);

  res =
      nghttp2_session_server_new(&ezsession->ngsession, ngcallbacks, ezsession);
  nghttp2_session_callbacks_del(ngcallbacks);
  if (res < 0) {
    return 1;
  }

  /* 16kb frame size */
  nghttp2_settings_entry siv[] = {
      {NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, ezsession->server->http2_settings.initial_window_size},
      {NGHTTP2_SETTINGS_MAX_FRAME_SIZE, ezsession->server->http2_settings.max_frame_size},
      {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, ezsession->server->http2_settings.max_concurrent_streams},
  };
  res =
      nghttp2_submit_settings(ezsession->ngsession, NGHTTP2_FLAG_NONE, siv, 2);
  if (res < 0) {
    assert(0);  // TODO
  }

  if (makenonblock(sockfd)) assert(0);

#ifdef _WIN32
  res = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&(DWORD){1}, sizeof(DWORD));
  if (res < 0) {
    assert(0);  // TODO
  }

  res = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (char *)&(BOOL){1}, sizeof(BOOL));
  if (res < 0) {
    assert(0);  // TODO
  }
#else

  res = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&(int){1}, sizeof(int));
  if (res < 0) {
    assert(0);  // TODO
  }

  res = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&(int){1}, sizeof(int));
  if (res < 0) {
    assert(0);  // TODO
  }
#endif

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
      abort();
  }
  /*  all seems to be successful. set the following */
  ezsession->lstreams = ezgrpc2_list_new(NULL);
#ifdef _WIN32
  RPC_STATUS wres = UuidCreate(&ezsession->session_uuid.uuid);
  assert(wres == RPC_S_OK);
#else
  uuid_generate_random(ezsession->session_uuid.uuid);
#endif

  return res;
}


int session_add(ezgrpc2_server *ezserver, ezgrpc2_list *levents, int listenfd) {
  struct sockaddr_storage sockaddr;
  struct pollfd *fds = ezserver->fds;
  EZNFDS nb_fds = ezserver->nb_fds;
  EZSOCKLEN sockaddr_len;


  sockaddr_len = sizeof(sockaddr);
  memset(&sockaddr, 0, sizeof(sockaddr));
  int confd = accept(listenfd, (struct sockaddr *)&sockaddr, &sockaddr_len);
  if (confd == -1) {
    EZGRPC2_LOG_ERROR(ezserver, "accept: %s", strerror(errno));
    return 1;
  }

  EZGRPC2_LOG_NORMAL(ezserver, "incoming %s connection\n", sockaddr.ss_family == AF_INET ? "ipv4" : "ipv6");

  EZNFDS ndx = get_unused_pollfd_ndx(fds, nb_fds);
  if (ndx == -1) {
    EZGRPC2_LOG_ERROR(ezserver, "@ %s: max client reached", __func__);
    shutdown(confd, SHUT_RDWR);
    close(confd);
    return 1;
  }

  if (session_create(&ezserver->sessions[ndx], confd, &sockaddr, sockaddr_len, ezserver)) {
    EZGRPC2_LOG_ERROR(ezserver, "@ %s: session_create failed", __func__);
    shutdown(confd, SHUT_RDWR);
    close(confd);
    return 1;
  }
  ezserver->sessions[ndx].session_uuid.index = ndx;
  ezgrpc2_event *event = event_new(EZGRPC2_EVENT_CONNECT, &ezserver->sessions[ndx].session_uuid);
  ezgrpc2_list_push_back(levents, event);

  fds[ndx].fd = confd;
  fds[ndx].events = POLLIN | POLLRDHUP;
  return 0;
}





int session_events(ezgrpc2_session *ezsession) {
  int res;
  if (!nghttp2_session_want_read(ezsession->ngsession) &&
      !nghttp2_session_want_write(ezsession->ngsession))
    return 1;

  if (nghttp2_session_want_read(ezsession->ngsession) &&
      (res = nghttp2_session_recv(ezsession->ngsession)))
    if (res) {
      ezgrpc2_server_log(ezsession->server, EZGRPC2_SERVER_LOG_ERROR, COLSTR("@ %s: nghttp2: %s. killing...\n", BHRED), __func__,
            nghttp2_strerror(res));
      return 1;
    }

  while (nghttp2_session_want_write(ezsession->ngsession)) {
    res = nghttp2_session_send(ezsession->ngsession);
    if (res) {
      EZGRPC2_LOG_ERROR(ezsession->server, "nghttp2: %s. killing...\n",
            nghttp2_strerror(res));
      return 1;
    }
  }
  return 0;
}
ezgrpc2_stream *stream_new(i32 stream_id) {
  ezgrpc2_stream *ezstream = calloc(1, sizeof(*ezstream));
  ezstream->lqueue_omessages = ezgrpc2_list_new(NULL);
  //ezstream->lheaders = ezgrpc2_list_new(NULL);
  ezstream->nb_headers = 0;
  ezstream->time = (uint64_t)time(NULL);
  ezstream->is_trunc = 0;
  ezstream->trunc_seek = 0;
  ezstream->stream_id = stream_id;
  return ezstream;
}
/* only frees what is passed. it does not free all the linked lists */
void stream_free(ezgrpc2_stream *ezstream) {
  free(ezstream->recv_data);
  free(ezstream->hgrpc_encoding);
  free(ezstream->hgrpc_accept_encoding);
  free(ezstream->hcontent_type);
  free(ezstream->hgrpc_timeout);

  for (size_t i = 0; i < ezstream->nb_headers; i++) {
    free(ezstream->headers[i].value);
    free(ezstream->headers[i].name);
  }

  free(ezstream);
}

// ezgrpc2_header *header_create(const u8 *name, size_t nlen, const u8 *value, size_t vlen) {
//  ezgrpc2_header *ezheader = malloc(sizeof(*ezheader));
//  if (ezheader == NULL)
//    return NULL;
//
//  ezheader->name = malloc(nlen);
//  ezheader->value = malloc(vlen);
//
//  memcpy(ezheader->name, name, nlen);
//  memcpy(ezheader->value, value, vlen);
//  ezheader->nlen = nlen;
//  ezheader->vlen = vlen;
//
//  return ezheader;
//}
//
//void header_free(ezgrpc2_header *ezheader) {
//  free(ezheader->name);
//  free(ezheader->value);
//}
