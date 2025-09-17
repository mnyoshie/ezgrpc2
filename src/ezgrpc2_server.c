#include "core.h"
#include "ezgrpc2_server.h"
#include "ezgrpc2_session_info.h"
#include "ezgrpc2_session_uuid.h"
#include "internal_helpers.h"
#define atlog(...) (void)0
#define ezlog(...) (void)0


extern ezgrpc2_session_info_t *session_info_new(void *);

EZGRPC2_API ezgrpc2_list_t *ezgrpc2_server_get_all_sessions_info(ezgrpc2_server_t *server) {
  ezgrpc2_list_t *lsession_info = ezgrpc2_list_new(NULL);
  assert(lsession_info != NULL);

  for (EZNFDS i = 2; i < server->nb_fds; i++) {
    if (server->fds[i].fd == -1)
      continue;
    ezgrpc2_session_info_t *session_info = session_info_new(NULL);
    ezgrpc2_list_push_back(lsession_info, session_info);
  }




  return lsession_info;

}

EZGRPC2_API ezgrpc2_session_info_t *ezgrpc2_server_get_session_info(ezgrpc2_server_t *server, ezgrpc2_session_uuid_t *session_uuid) {

  return NULL;
}

EZGRPC2_API ezgrpc2_server_t *ezgrpc2_server_new(
  const char *ipv4_addr, u16 ipv4_port,
  const char *ipv6_addr, u16 ipv6_port,
  int backlog,
  ezgrpc2_server_settings_t *server_settings,
  ezgrpc2_http2_settings_t *http2_settings) {
  struct sockaddr_in ipv4_saddr = {0};
  struct sockaddr_in6 ipv6_saddr = {0};
  ezgrpc2_server_t *server = NULL;
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

  if (server_settings == NULL) {
    ezgrpc2_server_settings_t *server_settings_ = ezgrpc2_server_settings_new(NULL);
    assert(server_settings_ != NULL);
    memcpy(&server->server_settings, server_settings_, sizeof(ezgrpc2_server_settings_t));
    ezgrpc2_server_settings_free(server_settings_);
  } else {
    memcpy(&server->server_settings, server_settings, sizeof(ezgrpc2_server_settings_t));
  }

  if (http2_settings == NULL) {
    ezgrpc2_http2_settings_t *http2_settings_= ezgrpc2_http2_settings_new(NULL);
    memcpy(&server->http2_settings, http2_settings_, sizeof(ezgrpc2_http2_settings_t));
    ezgrpc2_http2_settings_free(http2_settings_);
  } else {
    memcpy(&server->http2_settings, http2_settings, sizeof(ezgrpc2_http2_settings_t));
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

    if (setsockopt(ipv4_sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)&(int){1},
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

  server->nb_sessions = server->server_settings.max_connections + 2;
  server->sessions = calloc(server->server_settings.max_connections + 2, sizeof(*server->sessions));

  server->nb_fds = server->server_settings.max_connections + 2;
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


  /* start accepting connections */

  return server;

err:
  ezgrpc2_server_free(server);
  return NULL;
}










EZGRPC2_API int ezgrpc2_server_poll(
  ezgrpc2_server_t *server,
  ezgrpc2_list_t *levents,
  ezgrpc2_path_t *paths,
  size_t nb_paths,
  int timeout) {
  assert(levents != NULL);
  struct pollfd *fds = server->fds;
  const EZNFDS nb_fds = server->nb_fds;

  const int ready = EZPOLL(fds, nb_fds, timeout);
  if (ready <= 0)
    return ready;

    // add ipv4 session
  if (fds[0].revents & POLLIN)
    session_add(server, levents, fds[0].fd);
  // add ipv6 session
  if (fds[1].revents & POLLIN)
    session_add(server, levents, fds[1].fd);

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
      server->sessions[i].levents = levents;
      server->sessions[i].server_settings = &server->server_settings;
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


EZGRPC2_API void ezgrpc2_server_free(
  ezgrpc2_server_t *ezserver) {
  for (EZNFDS i = 0; i < 2; i++)
  {
    if (ezserver->fds[i].fd != -1) {
      close(ezserver->fds[i].fd);
    }
  }

  for (EZNFDS i = 2; i < ezserver->nb_fds; i++)
  {
    if (ezserver->fds[i].fd != -1) {
      shutdown(ezserver->fds[i].fd, SHUT_RDWR);
      close(ezserver->fds[i].fd);
      session_free(&ezserver->sessions[i]);
    }
  }
  free(ezserver->fds);
    
  free(ezserver->ipv4_addr);
  free(ezserver->ipv6_addr);
  free(ezserver->sessions);
  free(ezserver);
}

void ezgrpc2_server_log(ezgrpc2_server_t *server, uint32_t log_level, char *fmt, ...){
  if (!(log_level & server->server_settings.logging_level))
    return;
  va_list args;
  va_start(args, fmt);
  char dt[128] = {0};
  fprintf(server->server_settings.logging_fp, "[%s] ", ezgetdt(dt, sizeof(dt)));
  vfprintf(server->server_settings.logging_fp, fmt, args);
  va_end(args);
}
