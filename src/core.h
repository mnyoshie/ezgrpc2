#ifndef EZGRPC2_CORE_H
#define EZGRPC2_CORE_H
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdbool.h>
#include <nghttp2/nghttp2.h>

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <windows.h>
#  include <Rpc.h>

#  include "wepoll.h"
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
#else
#  include <uuid/uuid.h>
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
#endif /* _WIN32/elae */

#include "ansicolors.h"
#include "common.h"
#include "ezgrpc2.h"
#include "ezgrpc2_path.h"
#include "ezgrpc2_server_settings.h"
#include "ezgrpc2_server_settings_struct.h"
#include "ezgrpc2_session_info.h"
#include "ezgrpc2_session_uuid.h"
#include "ezgrpc2_http2_settings.h"
#include "ezgrpc2_http2_settings_struct.h"

#define EZGRPC2_API __attribute__((visibility("default")))


/* stores the values of a SETTINGS frame */
//typedef struct ezgrpc_settingsf_t ezgrpc_settingsf_t;
//struct ezgrpc_settingsf_t {
//  u32 header_table_size;
//  u32 enable_push;
//  u32 max_concurrent_streams;
//  u32 flow_control;
//  u32 max_frame_size;
//};


typedef struct ezgrpc2_stream_t ezgrpc2_stream_t;
struct ezgrpc2_stream_t {
  i32 stream_id;

  /* the time the stream is received by the server in
   * unix epoch */
  u64 time;
  ezgrpc2_list_t *lheaders;

  bool is_method_post : 1;
  bool is_scheme_http : 1;
  /* just a bool. if `te` is a trailer, or `te` does exists. */
  bool is_te_trailers : 1;
  /* just a bool. if content type is application/grpc* */
  bool is_content_grpc : 1;

  bool is_trunc : 1;

  /* stores `:path` */
  ezgrpc2_path_t *path;
  size_t path_index;

  /* recv_data */
  size_t recv_len;
  u8 *recv_data;

  size_t trunc_seek;
  ezgrpc2_list_t *lqueue_omessages;
};

typedef struct ezgrpc2_session_t ezgrpc2_session_t;
struct ezgrpc2_session_t {
  nghttp2_session *ngsession;
#ifdef _WIN32
  UUID session_uuid;
  SOCKET sockfd;
  int socklen;
#else
  uuid_t session_uuid;
  int sockfd;
  socklen_t socklen;
#endif
  ezgrpc2_server_settings_t *server_settings;


  struct sockaddr_storage sockaddr;
  ezgrpc2_list_t *levents;


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
 // ezgrpc_settingsf_t csettings;

  /* A pointer to the settings in ezgrpc2_server_t.settings
   */
  ezgrpc2_http2_settings_t server_http2_settings;
  ezgrpc2_http2_settings_t client_http2_settings;

  //size_t nb_open_streams;
  /* the streams in a linked lists. allocated when we are
   * about to receive a HEADERS frame */
  ezgrpc2_list_t *lstreams;

};

struct ezgrpc2_server_t {
  nghttp2_session *ngsession;
  ezgrpc2_server_settings_t server_settings;
  ezgrpc2_http2_settings_t http2_settings;
  u16 ipv4_port;
  u16 ipv6_port;

  /* ASCII strings which represent where address to bind to */
  char *ipv4_addr;
  char *ipv6_addr;

#ifdef _WIN32
  ULONG nb_fds;
#else
  nfds_t nb_fds;
#endif

  int epollfd;
  /* fds[0] reserve for ipv4 listen sockfd */
  /* fds[1] reserve for ipv6 listen sockfd */
  /* the rest are for client use */
  struct pollfd *fds;

  size_t nb_sessions;
  ezgrpc2_session_t *sessions;
};

#endif
