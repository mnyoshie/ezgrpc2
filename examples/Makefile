-include ../config.mk

CFLAGS += -I../src
LDFLAGS += -L../src -l:libezgrpc2.a -lnghttp2 -luuid -lpthread 

targets = hello_worldc.bin

all : $(targets)

clean :
	rm $(targets)

%c.bin : %c.o ../src/libezgrpc2.a
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

%c.o : %c.c
	$(CC) -c $(CFLAGS) $< -o $@
