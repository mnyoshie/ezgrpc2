-include ../../config.mk

CFLAGS += -I./unity -I..

all: list pthpool

list: ../ezgrpc2_list.o list.o unity/unity.o
	$(CC) $^ $(CFLAGS) -o $@

pthpool: ../ezgrpc2_pthpool.o ../ezgrpc2_list.o pthpool.o
	$(CC) $^ $(CFLAGS) -o $@

test: all
	./list
	./pthpool

clean:
	rm *.o list pthpool
