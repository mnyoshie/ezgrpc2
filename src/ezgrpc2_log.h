#ifndef EZGRPC2_LOG_H
#define EZGRPC2_LOG_H
#include <stdint.h>

#define EZGRPC2_LOG_ALL (uint32_t)0xffffffff
#define EZGRPC2_LOG_QUIET (uint32_t)(0)
#define EZGRPC2_LOG_INF0 (uint32_t)(1 << 1)
#define EZGRPC2_LOG_WARNING (uint32_t)(1 << 2)
#define EZGRPC2_LOG_ERROR (uint32_t)(1 << 3)
#define EZGRPC2_LOG_DEBUG (uint32_t)(1 << 4)

#endif
