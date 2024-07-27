LDFLAGS += -lnghttp2
LDFLAGS += -lpthread

CFLAGS += -I.

targets += examples/hello_world.bin

all: $(targets)

%.bin: %.o ezgrpc2.o list.o pthpool.o
	$(Q)$(CC) $^ $(LDFLAGS) -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm $(targets)


push:
	git add . && git commit -S && cat ~/kmnyoshie | \
		termux-clipboard-set && git push $(f)
