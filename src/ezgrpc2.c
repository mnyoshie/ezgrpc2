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
#include "ezgrpc2_server_settings.h"
#include "ezgrpc2_server_settings_struct.h"


#include "core.h"

#include "internal_nghttp2_callbacks.h"
#include "internal_helpers.h"


#define logging_fp stderr
// int logging_level;

//#define EZENABLE_IPV6
#define EZWINDOW_SIZE (1<<20)

#define atlog(fmt, ...) ezlog("@ %s: " fmt, __func__, ##__VA_ARGS__)
//#define atlog(fmt, ...) (void)0

//#define EZENABLE_DEBUG
#define ezlogm(...) \
  ezlogf(ezsession, __FILE__, __LINE__, __VA_ARGS__)




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



static int list_cmp_ezheader_name(const void *data, const void *userdata) {
  const ezgrpc2_header_t *a = data;
  const ezgrpc2_header_t *b = userdata;
  return a->nlen == b->nlen ?
      strncasecmp(a->name, b->name,
        a->nlen & b->nlen): 1;
}








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
  memcpy(&ezsession->server_settings, &server->server_settings, sizeof(ezgrpc2_server_settings_t));


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
      {NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, ezsession->server_settings.initial_window_size},
      {NGHTTP2_SETTINGS_MAX_FRAME_SIZE, ezsession->server_settings.max_frame_size},
      {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, ezsession->server_settings.max_concurrent_streams},
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
  ezgrpc2_server_settings_t *server_settings) {
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

  if (server_settings == NULL) {
    ezgrpc2_server_settings_t *server_settings_ = ezgrpc2_server_settings_new(NULL);
    assert(server_settings_ != NULL);
    memcpy(&server->server_settings, server_settings_, sizeof(ezgrpc2_server_settings_t));
    ezgrpc2_server_settings_free(server_settings_);
  } else {
    memcpy(&server->server_settings, server_settings, sizeof(ezgrpc2_server_settings_t));
  }
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
  int res;
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

  while (nghttp2_session_want_write(ezsession->ngsession)) {
    res = nghttp2_session_send(ezsession->ngsession);
    if (res) {
      ezlog(COLSTR("nghttp2: %s. killing...\n", BHRED),
            nghttp2_strerror(res));
      return 3;
    }
  }
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
  i8 *grpc_message = ezgrpc2_grpc_status_strstatus(status);
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
  if ((found = ezgrpc2_list_find(ezstream->lheaders, list_cmp_ezheader_name, ezheader))
      == NULL)
    return 3;

  return ezheader->value = found->value, ezheader->vlen = found->vlen, 0;
}


const char *ezgrpc2_license(void){
  static const char *license = 
    "Copyright (c) 2023-2025 M. N. Yoshie & Al-buharie Amjari\n"
    "\n"
    "Redistribution and use in source and binary forms, with or without\n"
    "modification, are permitted provided that the following conditions\n"
    "are met:\n"
    "\n"
    "  1. Redistributions of source code must retain the above copyright notice,\n"
    "  this list of conditions and the following disclaimer.\n"
    "\n"
    "  2. Neither the name of the copyright holder nor the names of its\n"
    "  contributors may be used to endorse or promote products derived from\n"
    "  this software without specific prior written permission.\n"
    "\n"
    "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n"
    "“AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n"
    "LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n"
    "A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT\n"
    "HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\n"
    "SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED\n"
    "TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR\n"
    "PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF\n"
    "LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING\n"
    "NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n"
    "SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n";
  return license;
}
