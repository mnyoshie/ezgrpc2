#include "ezgrpc2_grpc_status.h"

char *ezgrpc2_grpc_status_strstatus(ezgrpc2_grpc_status_t status) {
  switch (status) {
    case EZGRPC2_GRPC_STATUS_OK:
      return "ok";
    case EZGRPC2_GRPC_STATUS_CANCELLED:
      return "cancelled";
    case EZGRPC2_GRPC_STATUS_UNKNOWN:
      return "unknown";
    case EZGRPC2_GRPC_STATUS_INVALID_ARGUMENT:
      return "invalid argument";
    case EZGRPC2_GRPC_STATUS_DEADLINE_EXCEEDED:
      return "deadline exceeded";
    case EZGRPC2_GRPC_STATUS_NOT_FOUND:
      return "not found";
    case EZGRPC2_GRPC_STATUS_ALREADY_EXISTS:
      return "already exists";
    case EZGRPC2_GRPC_STATUS_PERMISSION_DENIED:
      return "permission denied";
    case EZGRPC2_GRPC_STATUS_UNAUTHENTICATED:
      return "unauthenticated";
    case EZGRPC2_GRPC_STATUS_RESOURCE_EXHAUSTED:
      return "resource exhausted";
    case EZGRPC2_GRPC_STATUS_FAILED_PRECONDITION:
      return "failed precondition";
    case EZGRPC2_GRPC_STATUS_OUT_OF_RANGE:
      return "out of range";
    case EZGRPC2_GRPC_STATUS_UNIMPLEMENTED:
      return "unimplemented";
    case EZGRPC2_GRPC_STATUS_INTERNAL:
      return "internal";
    case EZGRPC2_GRPC_STATUS_UNAVAILABLE:
      return "unavailable";
    case EZGRPC2_GRPC_STATUS_DATA_LOSS:
      return "data loss";
    default:
      return "(null)";
  }
}
