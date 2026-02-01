#ifndef EZGRPC2_HEADER_H
#define EZGRPC2_HEADER_H
#include <stdlib.h>
#include <stdint.h>

#define EZGRPC2_HEADER_CONTENT_TYPE_NULL 0
#define EZGRPC2_HEADER_CONTENT_TYPE_APPLICATION_GRPC 1
#define EZGRPC2_HEADER_CONTENT_TYPE_APPLICATION_GRPC_PROTOBUF 2
#define EZGRPC2_HEADER_CONTENT_TYPE_APPLICATION_GRPC_JSON 2
#define EZGRPC2_HEADER_CONTENT_TYPE_APPLICATION_GRPC_CUSTOM -1

/* content encoding */
enum ezgrpc2_content_encoding {
  EZGRPC2_GZIP = 1,
  EZGRPC2_DEFLATE = (1 << 1),
  EZGRPC2_SNAPPY = (1 << 2),
  EZGRPC2_ZSTD = (1 << 3),
  EZGRPC2_CUSTOM = (~(uint64_t)0)
};
typedef enum ezgrpc2_content_encoding ezgrpc2_content_encoding;

/* content type */
#define EZGRPC2_APPLICATION_GRPC (1)
#define EZGRPC2_APPLICATION_GRPC_PROTO (1 << 1)
#define EZGRPC2_APPLICATION_GRPC_JSON (1 << 2)
#define EZGRPC2_APPLICATION_GRPC_CUSTOM (~(uint64_t)0)

#define EZGRPC2_HEADER_GRPC_TIMEOUT_NULL 0
#define EZGRPC2_HEADER_GRPC_TIMEOUT_CUSTOM -1





typedef struct ezgrpc2_grpc_request_headers_t ezgrpc2_grpc_request_headers_t;
struct ezgrpc2_grpc_request_headers_t {
  int authority;
  int content_type;
  int grpc_encoding;
  int grpc_accept_encoding;
  int timeout;
  int timeout_unit;
};

typedef struct ezgrpc2_grpc_response_headers_t ezgrpc2_grpc_response_headers_t;
struct ezgrpc2_grpc_response_headers_t {
  int content_type;
  int grpc_encoding;
};



typedef struct ezgrpc2_header ezgrpc2_header;

/**
 *
 */
struct ezgrpc2_header {
  size_t namelen;
  char *name;
  size_t valuelen;
  char *value;
};

ezgrpc2_header *ezgrpc2_header_new(const u8 *name, size_t nlen, const u8 *value, size_t vlen);

void ezgrpc2_header_free(ezgrpc2_header *ezheader);
#endif
