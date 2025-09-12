#include <assert.h>
#include "core.h"
#include "ezgrpc2_header.h"
#include "internal_nghttp2_callbacks.h"
#include "internal_helpers.h"
#define atlog(...) (void)0
#define ezlog(...) (void)0

extern ezgrpc2_event_t *event_new(ezgrpc2_event_type_t type, ezgrpc2_session_uuid_t *session_uuid, ...);

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
  const ezgrpc2_header_t *a = data;
  const ezgrpc2_header_t *b = userdata;
  return a->nlen == b->nlen ?
      strncasecmp(a->name, b->name,
        a->nlen & b->nlen): 1;
}






EZNFDS get_unused_pollfd_ndx(struct pollfd *fds, EZNFDS nb_fds) {
  for (EZNFDS i = 2; i < nb_fds; i++)
    if (fds[i].fd == -1)
      return i;
  return -1;
}








void session_free(ezgrpc2_session_t *ezsession) {
  ezgrpc2_event_t *event = event_new(EZGRPC2_EVENT_DISCONNECT, 
      ezgrpc2_session_uuid_copy(&ezsession->session_uuid));
  ezgrpc2_list_push_back(ezsession->levents, event);

  nghttp2_session_terminate_session(ezsession->ngsession, 0);
  nghttp2_session_del(ezsession->ngsession);

  ezgrpc2_stream_t *ezstream;
  while ((ezstream = ezgrpc2_list_pop_front(ezsession->lstreams)) != NULL)
    stream_free(ezstream);

  //ezgrpc2_session_uuid_free(ezsession->session_uuid);
  //ezsession->session_uuid = NULL;

  memset(ezsession, 0, sizeof(*ezsession));
}








ezgrpc2_session_t *session_find(ezgrpc2_session_t *ezsessions, size_t nb_ezsessions, ezgrpc2_session_uuid_t *session_uuid) {
  for (size_t i = 0; i < nb_ezsessions; i++) 
    if (ezgrpc2_session_uuid_is_equal(&ezsessions[i].session_uuid, session_uuid))
      return ezsessions + i;

  return NULL;
}















int session_create(
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
  memcpy(&ezsession->server_http2_settings, &server->http2_settings, sizeof(ezgrpc2_server_settings_t));


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
      {NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, ezsession->server_http2_settings.initial_window_size},
      {NGHTTP2_SETTINGS_MAX_FRAME_SIZE, ezsession->server_http2_settings.max_frame_size},
      {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, ezsession->server_http2_settings.max_concurrent_streams},
  };
  res =
      nghttp2_submit_settings(ezsession->ngsession, NGHTTP2_FLAG_NONE, siv, 2);
  if (res < 0) {
    assert(0);  // TODO
  }

  if (makenonblock(sockfd)) assert(0);

  res = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&(int){1},
                   sizeof(int));
  if (res < 0) {
    assert(0);  // TODO
  }

  res = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&(int){1},
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
#ifdef _WIN32
  RPC_STATUS wres = UuidCreate(&ezsession->session_uuid);
  assert(wres == RPC_S_OK);
#else
  uuid_generate_random(ezsession->session_uuid);
#endif
  //ezsession->alive = 1;

  return res;
}


int session_add(ezgrpc2_server_t *ezserver, ezgrpc2_list_t *levents, int listenfd) {
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
  ezgrpc2_event_t *event = event_new(EZGRPC2_EVENT_CONNECT, 
      ezgrpc2_session_uuid_copy(&ezserver->sessions[ndx].session_uuid));
  ezgrpc2_list_push_back(levents, event);

  fds[ndx].fd = confd;
  fds[ndx].events = POLLIN | POLLRDHUP;
  return 0;
}





int session_events(ezgrpc2_session_t *ezsession) {
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
ezgrpc2_stream_t *stream_new(i32 stream_id) {
  ezgrpc2_stream_t *ezstream = calloc(1, sizeof(*ezstream));
  ezstream->lqueue_omessages = ezgrpc2_list_new(NULL);
  ezstream->lheaders = ezgrpc2_list_new(NULL);
  ezstream->time = (uint64_t)time(NULL);
  ezstream->is_trunc = 0;
  ezstream->trunc_seek = 0;
  ezstream->stream_id = stream_id;
  return ezstream;
}
/* only frees what is passed. it does not free all the linked lists */
void stream_free(ezgrpc2_stream_t *ezstream) {
  atlog("stream %d freed\n", ezstream->stream_id);
  ezgrpc2_header_t *ezheader;

  while ((ezheader = ezgrpc2_list_pop_front(ezstream->lheaders)) != NULL) {
    ezgrpc2_header_free(ezheader);
  }
  free(ezstream->recv_data);
  ezgrpc2_list_free(ezstream->lheaders);
  free(ezstream);
}

// ezgrpc2_header_t *header_create(const u8 *name, size_t nlen, const u8 *value, size_t vlen) {
//  ezgrpc2_header_t *ezheader = malloc(sizeof(*ezheader));
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
//void header_free(ezgrpc2_header_t *ezheader) {
//  free(ezheader->name);
//  free(ezheader->value);
//}
