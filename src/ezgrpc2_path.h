#ifndef EZGRPZ_PATH_H
#define EZGRPZ_PATH_H
#include <stdint.h>
#include "ezgrpc2_header.h"

typedef struct ezgrpc2_path ezgrpc2_path;
/**
 * A path to poll. To be passed to :c:func:`ezgrpc2_server_poll()`.
 */
struct ezgrpc2_path {
  /**
   * Path to listen to. Must not contain an anchor or a query (# or ?)
   */
  char path[CONFIG_PATH_MAX_LEN];

  /**
   * User defined userdata
   */
  void *userdata;

  /* {gzip,deflate,snappy,zstd} */
  ezgrpc2_content_encoding accept_content_encoding;
  /* application/grpc[+{proto,json,custom}] */
  uint64_t accept_content_type;
};
#endif
