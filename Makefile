LDFLAGS += -lnghttp2
LDFLAGS += -lpthread
LDFLAGS += -lcrypto
LDFLAGS += -luuid

.PHONY: push test

ifeq ($(OS),Windows_NT)
	LDFLAGS += -lWs2_32
endif

LDFLAGS += -fsanitize=address


CFLAGS += -I. -ggdb3 -fsanitize=address
CFLAGS += -Wall
#CFLAGS += -Wextra

tests = tests/test_listc.bin
tests += tests/test_pthpoolc.bin
tests += tests/test_ezgrpc2c.bin

targets = examples/hello_worldc.bin
targets += $(tests)
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

test:
	./tests/test_listc.bin
	./tests/test_pthpoolc.bin
	./tests/test_ezgrpc2c.bin

push:
	git add . && git commit -S && cat ~/kmnyoshie | \
		termux-clipboard-set && git push $(f)
