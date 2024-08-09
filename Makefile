LDFLAGS += -lnghttp2
LDFLAGS += -lpthread
LDFLAGS += -lcrypto
LDFLAGS += -luuid

ifeq ($(OS),Windows_NT)
	LDFLAGS += -lWs2_32
endif

LDFLAGS += -fsanitize=address


CFLAGS += -I. -ggdb3 -fsanitize=address
CFLAGS += -Wall
#CFLAGS += -Wextra

targets += examples/hello_worldc.bin
#targets += examples/hello_worldcpp.bin

all: $(targets)

%c.bin: %c.o ezgrpc2.o list.o pthpool.o
	$(Q)$(CC) $^ $(LDFLAGS) -o $@

%cpp.bin: %cpp.cpp ezgrpc2.cpp  ezgrpc2.o list.o pthpool.o
	$(Q)$(CXX) $^ $(CFLAGS) $(LDFLAGS) -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm $(targets)


push:
	git add . && git commit -S && cat ~/kmnyoshie | \
		termux-clipboard-set && git push $(f)
