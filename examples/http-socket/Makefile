all: http-example
CONTIKI=../..
MODULES += os/net/app-layer/http-socket

CONTIKI_WITH_IPV6 = 1
CONTIKI_WITH_RPL_LITE = 0
CFLAGS += -DUIP_CONF_TCP=1
include $(CONTIKI)/Makefile.include
