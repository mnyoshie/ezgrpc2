-include ../config.mk

CFLAGS += -I../src
LDFLAGS += -L../src -l:libezgrpc2.a -lnghttp2 -luuid -lpthread 

CFLAGS += $(EXTRACFLAGS)

targets = hello_worldc.bin

all : $(targets)

clean :
	rm $(targets)

%c.bin : %c.o
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

%c.o : %c.c
	$(CC) -c $(CFLAGS) $< -o $@
