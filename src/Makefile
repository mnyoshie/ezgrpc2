-include ../config.mk

LDFLAGS += -lnghttp2
LDFLAGS += -lpthread
LDFLAGS += -lcrypto
LDFLAGS += -luuid
CFLAGS += -I.
CFLAGS += -fPIC
CFLAGS += $(EXTRACFLAGS)

.PHONY: push test html host_html


CFLAGS += -Wall

#CFLAGS += -Wextra

tests = tests/test_listc.bin
tests += tests/test_pthpoolc.bin
tests += tests/test_ezgrpc2c.bin

targets = libezgrpc2.so
targets += libezgrpc2.a

objects = ezgrpc2.o
objects += ezgrpc2_event.o
objects += ezgrpc2_list.o
objects += ezgrpc2_pthpool.o
objects += ezgrpc2_session_uuid.o
#targets += examples/hello_worldcpp.bin

all: $(targets)

libezgrpc2.a : $(objects)
	$(Q)$(AR) rcs $@ $^

libezgrpc2.so : $(objects)
	$(Q)$(CC) -shared $(CFLAGS) $^ -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm $(targets) *.o
	$(MAKE) -C tests clean || true

test:
	make -C tests test

