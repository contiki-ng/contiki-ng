CONTIKI_PROJECT = packet-injector
all: $(CONTIKI_PROJECT)

PLATFORM_ONLY = native
TARGET = native

MODULES += os/net/app-layer/coap
MODULES += os/net/mac/ble

CONTIKI = ../../../
include $(CONTIKI)/Makefile.include
