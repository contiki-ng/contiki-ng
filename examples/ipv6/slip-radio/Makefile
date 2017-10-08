CONTIKI_PROJECT=slip-radio
all: $(CONTIKI_PROJECT)
MODULES += os/services/slip-cmd

ifeq ($(TARGET),)
  -include Makefile.target
endif

CONTIKI=../../..

PROJECT_SOURCEFILES += slip-net.c
ifeq ($(TARGET),sky)
  PROJECT_SOURCEFILES += slip-radio-cc2420.c slip-radio-sky-sensors.c
endif
ifeq ($(TARGET),nooliberry)
  PROJECT_SOURCEFILES += slip-radio-rf230.c
endif

# custom net layer, but with IPv6 enabled
MAKE_NET = MAKE_NET_IPV6
CONTIKI_WITH_RPL = 0
include $(CONTIKI)/Makefile.include
