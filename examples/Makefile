-include ../config.mk

CFLAGS += -I../src
LDFLAGS += -L../src -l:libezgrpc2.a -lnghttp2 -luuid -lpthread 

targets = multi_threaded.bin
targets += single_threaded.bin

all : $(targets)

clean :
	rm $(targets)

%.bin : %.o ../src/libezgrpc2.a
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
