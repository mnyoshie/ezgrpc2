#ifndef EZGRPZ_PATH_H
#define EZGRPZ_PATH_H
#include "ezgrpc2_list.h"

typedef struct ezgrpc2_path_t ezgrpc2_path_t;
/**
 * A path to poll. To be passed to :c:func:`ezgrpc2_server_poll()`.
 */
struct ezgrpc2_path_t {
  /**
   * Path to listen to. Must not contain an anchor or a query (# or ?)
   */
  char *path;

  /**
   * User defined userdata
   */
  void *userdata;

  /**
   * This is a list of :c:struct:`ezgrpc2_event_t` and it
   * contains events for this specific path.
   *
   * Cast the return of :c:func:`ezgrpc2_list_pop_first()` to a pointer to :c:struct:`ezgrpc2_event_t` when its argument id ``levents``
   */
  ezgrpc2_list_t *levents;
};
#endif
