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
};
#endif
