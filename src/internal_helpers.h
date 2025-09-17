#ifndef EZGRPC2_INTERNAL_HELPERS_H
#define EZGRPC2_INTERNAL_HELPERS_H

#include "core.h"

#ifdef _WIN32
int makenonblock(SOCKET sockfd);
#else
int makenonblock(int sockfd);
#endif

int session_add(ezgrpc2_server_t *ezserver, ezgrpc2_list_t *levents, int listenfd);
int session_events(ezgrpc2_session_t *ezsession);
void session_free(ezgrpc2_session_t *ezsession);
ezgrpc2_session_t *session_find(ezgrpc2_session_t *ezsessions, size_t nb_ezsessions, ezgrpc2_session_uuid_t *session_uuid);
int session_create(
    ezgrpc2_session_t *ezsession,
    EZSOCKET sockfd,
    struct sockaddr_storage *sockaddr,
    EZSOCKLEN socklen,
    ezgrpc2_server_t *server);

void stream_free(ezgrpc2_stream_t *stream);
ezgrpc2_stream_t *stream_new(i32 stream_id);

int list_cmp_ezheader_name(const void *data, const void *userdata);
void ezlog(char *fmt, ...);
void ezflog(FILE *fp, char *fmt, ...);
char *ezgetdt(char *buf, size_t len);


EZNFDS get_unused_pollfd_ndx(struct pollfd *fds, EZNFDS nb_fds);


#endif
