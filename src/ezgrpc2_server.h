#ifndef EZGRPC2_SERVER_H
#define EZGRPC2_SERVER_H
#include <stdint.h>
#include "ezgrpc2_path.h"
#include "ezgrpc2_http2_settings.h"
#include "ezgrpc2_server_settings.h"
#include "ezgrpc2_session_info.h"
#include "ezgrpc2_session_uuid.h"

#define EZGRPC2_SERVER_LOG_ALL (uint32_t)0xffffffff
#define EZGRPC2_SERVER_LOG_QUIET (uint32_t)(0)
#define EZGRPC2_SERVER_LOG_NORMAL (uint32_t)(1)
#define EZGRPC2_SERVER_LOG_WARNING (uint32_t)(1 << 1)
#define EZGRPC2_SERVER_LOG_ERROR (uint32_t)(1 << 2)
#define EZGRPC2_SERVER_LOG_DEBUG (uint32_t)(1 << 3)
#define EZGRPC2_SERVER_LOG_TRACE (uint32_t)(1 << 4)

#define EZGRPC2_LOG_NORMAL(server, ...)  ezgrpc2_server_log(server, EZGRPC2_SERVER_LOG_NORMAL, __VA_ARGS__)
#define EZGRPC2_LOG_ERROR(server, fmt, ...)  ezgrpc2_server_log(server, EZGRPC2_SERVER_LOG_ERROR, COLSTR("error: " fmt, BHRED), ##__VA_ARGS__)
#define EZGRPC2_LOG_WARNING(server, fmt, ...)  ezgrpc2_server_log(server, EZGRPC2_SERVER_LOG_WARNING, COLSTR("warning: " fmt, BHYEL), ##__VA_ARGS__)

#ifdef EZGRPC2_DEBUG
#define EZGRPC2_LOG_DEBUG(server, fmt, ...)  ezgrpc2_server_log(server, EZGRPC2_SERVER_LOG_DEBUG, "@%s: " fmt, __func__, ##__VA_ARGS__)
#else
#define EZGRPC2_LOG_DEBUG(server, ...) do {} while (0)
#endif

#ifdef EZGRPC2_TRACE
#define EZGRPC2_LOG_TRACE(server, fmt, ...)  ezgrpc2_server_log(server, EZGRPC2_SERVER_LOG_TRACE, "@%s: " fmt, __func__,  ##__VA_ARGS__)
#else
#define EZGRPC2_LOG_TRACE(server, ...) do {} while (0)
#endif

typedef struct ezgrpc2_server_t ezgrpc2_server_t;

/**
 * An opaque server context struct type returned by :c:func:`ezgrpc2_server_new()`.
 */
struct ezgrpc2_server_t;

/**
 * The :c:func:`ezgrpc2_server_new()` function creates a ezserver context,
 * binding to the associated ``ipv4_addr`` and ``ipv6_addr`` if it's not
 * ``NULL``.
 *
 * At no point ``ipv4_addr`` and ``ipv6_addr`` be ``NULL.``
 *
 * :param settings: Unused. Reserved for future implementation. Must be set to ``NULL``.
 *
 * :returns:
 *
 *     * On success, a pointer to the opaque :c:struct:`ezgrpc2_server_t`
 *
 *     * On failure, ``NULL``.
 *
 * Example 1:
 *
 * .. code-block:: C
 *
 *    ezgrpc2_server_t *ezgrpc2_server_new("0.0.0.0", 8080, "::", 8080, 16, NULL);
 *
 */
ezgrpc2_server_t *ezgrpc2_server_new(
  const char *ipv4_addr, u16 ipv4_port,
  const char *ipv6_addr, u16 ipv6_port,
  int backlog,
  ezgrpc2_server_settings_t *server_settings,
  ezgrpc2_http2_settings_t *http2_settings);


/**
 * The :c:func:`ezgrpc2_server_poll()` function polls the server for any clients making a request to paths
 * appointed by ``paths``.
 *
 * If the requested path by the client is not found, a trailer, :c:enumerator:`EZGRPC2_GRPC_STATUS_UNIMPLEMENTED`
 * is automatically sent and closes the associated stream. No event is generated.
 *
 * :returns:
 *
 *    * On an event, a value greater than 0.
 *
 *    * On no event, 0.
 *
 *    * On error, a negative value.
 *
 * .. note::
 *    The :c:member:`ezgrpc2_path_t.levents` needs to be
 *    initialized first with, :c:func:`ezgrpc2_list_init()`.
 *
 *
 */
int ezgrpc2_server_poll(
  ezgrpc2_server_t *server,
  ezgrpc2_list_t *levents,
  ezgrpc2_path_t *paths,
  size_t nb_paths,
  int timeout);

void ezgrpc2_server_free(
  ezgrpc2_server_t *server);

ezgrpc2_list_t *ezgrpc2_server_get_all_sessions_info(ezgrpc2_server_t *server);

ezgrpc2_session_info_t *ezgrpc2_server_get_session_info(ezgrpc2_server_t *server, ezgrpc2_session_uuid_t *session_uuid);


void ezgrpc2_server_log(ezgrpc2_server_t *server, uint32_t log_level, char *fmt, ...);


#endif

