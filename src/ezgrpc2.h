/* ezgrpc2.c - A (crude) gRPC server in C. */

#ifndef EZGRPC2_H
#define EZGRPC2_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>
#include "common.h"

#ifndef EZGRPC2_API
#define EZGRPC2_API __attribute__((visibility("default")))
#endif

#include "ezgrpc2_arena_event.h"
#include "ezgrpc2_arena_message.h"
#include "ezgrpc2_event.h"
#include "ezgrpc2_events.h"
#include "ezgrpc2_global.h"
#include "ezgrpc2_http2_settings.h"
#include "ezgrpc2_server.h"
#include "ezgrpc2_server_settings.h"
#include "ezgrpc2_session.h"
#include "ezgrpc2_session_uuid.h"
#include "ezgrpc2_mem.h"
#include "ezgrpc2_message.h"
#include "ezgrpc2_messages.h"
#include "ezgrpc2_grpc_status.h"
#include "ezgrpc2_header.h"

#endif /* EZGRPC2_H */
