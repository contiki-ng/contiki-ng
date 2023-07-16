CONTIKI = ../../..

CC = gcc
CFLAGS += -Wall -g
CFLAGS += -I.
CFLAGS += -I$(CONTIKI)/os

MEMB_C = $(CONTIKI)/os/lib/memb.c

ARCH = native

all: test-memb

memb.o: $(MEMB_C)
	$(CC) $(CFLAGS) -c $< -o $@

test-memb: test-memb-api.o memb.o
	$(CC) $^ -o $@

clean:
	rm -rf test-memb test-memb.* *.o build
