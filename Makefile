LDFLAGS += -lnghttp2
LDFLAGS += -lpthread
LDFLAGS += -lcrypto
LDFLAGS += -luuid

.PHONY: push test html host_html

ifeq ($(OS),Windows_NT)
	LDFLAGS += -lWs2_32
endif

ifeq ($(HAVE_SECCOMP),1)
  LDFLAGS += -lseccomp
  CFLAGS += -DHAVE_SECCOMP
endif

ifeq ($(DEBUG),1)
  LDFLAGS += -fsanitize=address
  CFLAGS += -fsanitize=address
endif

CFLAGS += -I. -ggdb3
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

html:
	$(MAKE) -C docs html

host_html: html
	php -t ./docs/build/html/ -S localhost:8080

push:
	git add . && git commit -S && cat ~/kmnyoshie | \
		termux-clipboard-set && git push $(f)

