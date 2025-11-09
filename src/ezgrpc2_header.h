#ifndef EZGRPC2_HEADER_H
#define EZGRPC2_HEADER_H
#include <stdlib.h>

#define EZGRPC2_HEADER_CONTENT_TYPE_NULL 0
#define EZGRPC2_HEADER_CONTENT_TYPE_APPLICATION_GRPC 1
#define EZGRPC2_HEADER_CONTENT_TYPE_APPLICATION_GRPC_PROTOBUF 2
#define EZGRPC2_HEADER_CONTENT_TYPE_APPLICATION_GRPC_JSON 2
#define EZGRPC2_HEADER_CONTENT_TYPE_APPLICATION_GRPC_CUSTOM -1

#define EZGRPC2_HEADER_GRPC_ENCODING_NULL 0
#define EZGRPC2_HEADER_GRPC_ENCODING_GZIP 1
#define EZGRPC2_HEADER_GRPC_ENCODING_DEFLATE 2
#define EZGRPC2_HEADER_GRPC_ENCODING_SNAPPY 3
#define EZGRPC2_HEADER_GRPC_ENCODING_ZSTD 4
#define EZGRPC2_HEADER_GRPC_ENCODING_CUSTOM -1

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



typedef struct ezgrpc2_header_t ezgrpc2_header_t;

/**
 *
 */
struct ezgrpc2_header_t {
  size_t namelen;
  char *name;
  size_t valuelen;
  char *value;
};

ezgrpc2_header_t *ezgrpc2_header_new(const u8 *name, size_t nlen, const u8 *value, size_t vlen);

void ezgrpc2_header_free(ezgrpc2_header_t *ezheader);
#endif
