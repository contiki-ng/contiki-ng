CONTIKI_PROJECT=slip-radio
all: $(CONTIKI_PROJECT)
MODULES += os/services/slip-cmd

CONTIKI=../..
-include $(CONTIKI)/Makefile.identify-target

PROJECT_SOURCEFILES += slip-net.c
ifeq ($(TARGET),sky)
  PROJECT_SOURCEFILES += slip-radio-cc2420.c slip-radio-sky-sensors.c
endif

# custom net layer, but with IPv6 enabled
MAKE_NET = MAKE_NET_IPV6
MAKE_ROUTING = MAKE_ROUTING_NONE
include $(CONTIKI)/Makefile.include
