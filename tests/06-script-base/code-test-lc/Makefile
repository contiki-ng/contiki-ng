CONTIKI = ../../..

CC ?= gcc
CFLAGS += -Wall -g -I/user/local/include
CFLAGS += -I$(CONTIKI)/os

all: test_lc_switch test_lc_addrlabels

test-lc-switch: test.c
	$(CC) $(CFLAGS) -DTEST_LC_SWITCH -o $@ test.c

test-lc-addrlabels: test.c
	$(CC) $(CFLAGS) -DTEST_LC_ADDRLABELS -o $@ test.c

clean:
	rm -rf test-lc-switch* test-lc-addrlabels*
