-include ../../config.mk

CFLAGS += -I./unity -I..
CFLAGS += $(EXTRACFLAGS)

all: list pthpool

list: ../ezgrpc2_list.o list.o unity/unity.o
	$(CC) $^ $(EXTRACFLAGS) -o $@

pthpool: ../ezgrpc2_pthpool.o ../ezgrpc2_list.o pthpool.o
	$(CC) $^ $(EXTRACFLAGS) -o $@

test: all
	./list
	./pthpool

clean:
	rm *.o list pthpool
