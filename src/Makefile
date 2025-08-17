-include ../config.mk

LDFLAGS += -lnghttp2
LDFLAGS += -lpthread
LDFLAGS += -lcrypto
LDFLAGS += -luuid
CFLAGS += -I.
CFLAGS += -fPIC
CFLAGS += $(EXTRACFLAGS)

.PHONY: push test html host_html

ifeq ($(OS),Windows_NT)
	LDFLAGS += -lWs2_32
endif

ifeq ($(HAVE_SECCOMP),1)
  LDFLAGS += -lseccomp
  CFLAGS += -DHAVE_SECCOMP
endif

ifeq ($(DEBUG),1)
  CFLAGS += -ggdb3
endif

ifeq ($(ENABLE_ASAN),1)
  LDFLAGS += -fsanitize=address
  CFLAGS += -fsanitize=address
endif

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
	$(MAKE) -C tests clean

test:
	make -C tests test

