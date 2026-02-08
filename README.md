# EZgRPC2

> [!NOTE]
> This repository is under development and is not for production use;
> we’re actively tightening the bolts and refueling the engine, but we
> wouldn’t recommend using it to commute to work just yet as there isn't
> a stable release yet and the API changes may break your code (if not your
> bones!). However, the hangar doors are wide open! You are encouraged
> to kick the tires, poke the wires, and see how high it flies in your local
> environment. If you manage to break the sound barrier (or just the code),
> let us know.


This library doesn't necessarily makes the implementation of gRPC server
easier, in fact, it makes it harder.

This library does not provide a way to serialized message and is not
dependent on any serialization library such as protobuf or flatbuffers. If
you want your message to travel across the world and beyond, be prepare
to hand-weave your own custom encoder-decoder like an 18th century
data smith.

## SSL/TLS support

To be determine.

## Architecture

This architecture was inspired by `poll(2)`, but instead of polling fds
and returning events such as POLLIN, you register a couple of paths and it
gives events of `EVENT_CONNECT`, `EVENT_DISCONNECT`, `EVENT_MESSAGE`,
`EVENT_DATALOSS` and `EVENT_CANCEL` to specific stream ids. In essence,
you need process this event struct:
```c
struct ezgrpc2_event {
  union {
    ezgrpc2_event_message message;
    ezgrpc2_event_dataloss dataloss;
    ezgrpc2_event_cancel cancel;
  };
  ezgrpc2_event_type type;
  ezgrpc2_session_uuid session_uuid;
  void *userdata;
};
```

## Building

Linux dependencies: `nghttp2 pthreads libuuid`

Windows dependencies: `nghttp2 pthreads`

Once the dependencies are installed, building on Linux and Windows (msys2
ucrt64) are pretty much the same. Run make:
```
make
```

## Usage

Includes:
```c
#include "ezgrpc2.h"
```

At startup of your application, please initialize the library by calling,
`ezgrpc2_global_init(0);`. Then call `ezgrpc2_global_cleanup();` once you're done
using the library.

Creating a server:
```c
uint16 port = 19009;
int backlog = 16;
ezgrpc2_server *server = ezgrpc2_server_new("0.0.0.0", port, "::", port, backlog, NULL, NULL);
```

Setting up service:
```c
ezgrpc2_server_register_path(server, "/test.yourAPI/whatever_service", path_userdata, 0, 0);
```

Polling for events:
```c
ezgrpc2_events *events = ezgrpc2_events_new(NULL);
int timeout = 10000;
int res = ezgrpc2_server_poll(server, levents, timeout);
```

Handle events:
```c
if (res > 0) {
  ezgrpc2_event *event;
  size_t nb_event;
  while ((event = ezgrpc2_events_read(events, &nb_event)) != NULL) {
    for (size_t i = 0; i < nb_event; i++) {
      switch (event[i].type) {
      case EZGRPC2_EVENT_MESSAGE: {
        printf("event message on stream id %d\n", event[i].message.stream_id);
        ezgrpc2_message *message;
        while ((message = ezgrpc2_messages_pop(event[i].message.messages)) != NULL) {
          printf("  message: compressed flag %d, len %zu, data %p\n", message->is_compressed, message->len, message->data);
          ezgrpc2_message_free(message);
        }
        ezgrpc2_messages_free(event[i].message.messages);
        } break;
      case EZGRPC2_EVENT_DATALOSS:
        printf("event dataloss on stream id %d\n", event[i].dataloss.stream_id);
        break;
      case EZGRPC2_EVENT_CANCEL:
        printf("event cancel on stream id %d\n", event[i].cancel.stream_id);
        break;
      } /* switch */
    } /* for */
    /* free event block */
    ezgrpc2_free(event);
  }
}
```
`sesssion_uuid` is a 16 bytes universally unique identifier, but
this may vary between other systems;


Creating a message:
```c
char greet[] = "Hello mom!";
ezgrpc2_message *message = ezgrpc2_message_new2(0, greet, sizeof(greet));

ezgrpc2_messages *messages = ezgrpc2_messages_new(NULL);
ezgrpc2_messages_push(messages, message);
```

Sending a message:
```c
ezgrpc2_server_session_stream_send(server, event->session_uuid, event->message.stream_id, messages);
ezgrpc2_messages_free(messages);
```

Ending a stream:
```c
ezgrpc2_server_session_stream_end(server, event->session_uuid, event->message.stream_id, EZGRPC2_GRPC_STATUS_OK);
```

when you're done using the server:
```c
ezgrpc2_server_free(server);
```

see `https://github.com/mnyoshie/ezgrpc2/blob/master/examples` for a complete
MWE server.

## About streaming and unary services

The EZgRPC2 server library have no such concept of streaming or unary
services and that all it knows is to accept messages if it exists in
the `path` we're polling using `ezgrpc2_server_poll()`. But, you can
enforce that a service is unary by ending the stream immediately with the status
`EZGRPC2_GRPC_STATUS_INVALID_ARGUMENT` if you received more than one messages
in a stream.

Even though the HTTP2 standard permits closing a stream with an empty DATA
frame, this is illegal in EZgRPC2, because if a service is unary, it might
be misunderstood that further messages will be received in that stream
and get immediately closed with `EZGRPC2_GRPC_STATUS_INVALID_ARGUMENT`.


## License
```
ezgrpc2 - A grpc server without the extra fancy features.
https://github.com/mnyoshie/ezgrpc2

Copyright (c) 2023-2025 M. N. Yoshie
Copyright (c) 2023-2025 Al-buharie Amjari

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
“AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

And if you're in windows, it includes an additional license from using
wepoll.
```
wepoll - epoll for Windows
https://github.com/piscisaureus/wepoll

Copyright 2012-2020, Bert Belder <bertbelder@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```
