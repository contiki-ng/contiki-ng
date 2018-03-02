CONTIKI_PROJECT = root intermediate sink
all: $(CONTIKI_PROJECT)

# nrf52dk only supports slave mode, i.e., with no routing
PLATFORMS_EXCLUDE = nrf52dk

CONTIKI = ../..

MODULES += os/net/ipv6/multicast

MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC
include $(CONTIKI)/Makefile.include
