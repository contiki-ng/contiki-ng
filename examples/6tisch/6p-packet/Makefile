CONTIKI_PROJECT      = sixp-node
PROJECT_SOURCEFILES += test-sf.c

PLATFORMS_EXCLUDE = sky nrf52dk native simplelink
BOARDS_EXCLUDE = srf06/cc13xx launchpad/cc1310 launchpad/cc1350 sensortag/cc2650 sensortag/cc1350

CONTIKI  = ../../../

MAKE_MAC = MAKE_MAC_TSCH
MAKE_ROUTING = MAKE_ROUTING_NULLROUTING
MODULES += os/net/mac/tsch/sixtop

all: $(CONTIKI_PROJECT)
include $(CONTIKI)/Makefile.include
