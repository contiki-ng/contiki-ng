all: udp-client udp-server udp-client-packet-debug
CONTIKI=../..

ifdef SERVER_REPLY
CFLAGS+=-DSERVER_REPLY=$(SERVER_REPLY)
endif
ifdef PERIOD
CFLAGS+=-DPERIOD=$(PERIOD)
endif

include $(CONTIKI)/Makefile.include
