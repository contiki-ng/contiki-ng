CONTIKI_PROJECT=slip-radio
all: $(CONTIKI_PROJECT)
MODULES += os/services/slip-cmd

# slip-radio is only intended for platforms with SLIP support
PLATFORMS_EXCLUDE = native nrf52dk

CONTIKI=../..
include $(CONTIKI)/Makefile.identify-target

MODULES_REL += $(TARGET)
PROJECT_SOURCEFILES += slip-net.c

# custom net layer, but with IPv6 enabled
MAKE_NET = MAKE_NET_IPV6
MAKE_ROUTING = MAKE_ROUTING_NULLROUTING
include $(CONTIKI)/Makefile.include
