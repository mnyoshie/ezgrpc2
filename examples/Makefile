-include ../config.mk

CFLAGS += -I../src
LDFLAGS += -L../src -l:libezgrpc2.a -lnghttp2 -lpthread 

CFLAGS+=$(EXTRACFLAGS)

targets = multi_threaded.bin
targets += single_threaded.bin

ifeq ($(OS),Windows_NT)
	LDFLAGS += -lWs2_32
	LDFLAGS += -lRpcrt4
else
	LDFLAGS += -luuid
endif

ifeq ($(HAVE_SECCOMP),1)
  LDFLAGS += -lseccomp
  CFLAGS += -DHAVE_SECCOMP
endif

all : $(targets)

clean :
	rm $(targets)

%.bin : %.o ../src/libezgrpc2.a
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
