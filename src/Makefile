-include ../config.mk

CFLAGS += -I.
CFLAGS += -fPIC
CFLAGS += -fvisibility=hidden
CFLAGS += $(EXTRACFLAGS)

.PHONY: push test html host_html


CFLAGS += -Wall

#CFLAGS += -Wextra

tests = tests/test_listc.bin
tests += tests/test_pthpoolc.bin
tests += tests/test_ezgrpc2c.bin

ifeq ($(OS),Windows_NT)
  targets = libezgrpc2.dll
else
  targets = libezgrpc2.so
endif
targets += libezgrpc2.a

objects = thpool.o

objects += ezgrpc2_arena_event.o
objects += ezgrpc2_arena_message.o
objects += ezgrpc2_event.o
objects += ezgrpc2_global.o
objects += ezgrpc2_grpc_status.o
objects += ezgrpc2_header.o
objects += ezgrpc2_http2_settings.o
objects += ezgrpc2_list.o
objects += ezgrpc2_message.o
#objects += ezgrpc2_messages.o
objects += ezgrpc2_pthpool.o
#objects += ezgrpc2_ring.o
objects += ezgrpc2_server.o
objects += ezgrpc2_server_settings.o
objects += ezgrpc2_session.o
objects += ezgrpc2_session_info.o
objects += ezgrpc2_session_uuid.o
objects += internal_helpers.o
objects += internal_nghttp2_callbacks.o
ifdef ($(OS),Windows_NT)
  objects += wepoll.o
endif
#targets += examples/hello_worldcpp.bin

all: $(targets)

libezgrpc2.a : $(objects)
	$(Q)$(AR) rcs $@ $^

libezgrpc2.so : $(objects)
	$(Q)$(CC) -shared -lnghttp2 $^ -o $@

libezgrpc2.dll : $(objects)
	$(Q)$(CC) -shared $(CFLAGS) $^ -lnghttp2 -lwinpthread -lRpcrt4 -lWs2_32 -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm $(targets) *.o
	$(MAKE) -C tests clean

test:
	make -C tests test

