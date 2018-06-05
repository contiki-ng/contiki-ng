CONTIKI_PROJECT = mqtt-demo
all: $(CONTIKI_PROJECT)

PLATFORMS_ONLY = nrf52dk

MAKE_ROUTING = MAKE_ROUTING_NULLROUTING

MODULES += os/net/app-layer/mqtt

CONTIKI=../../../..
include $(CONTIKI)/Makefile.include
