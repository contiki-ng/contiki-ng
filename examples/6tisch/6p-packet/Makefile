CONTIKI_PROJECT      = sixp-node
PROJECT_SOURCEFILES += test-sf.c

PLATFORMS_EXCLUDE = sky z1 nrf52dk native simplelink

CONTIKI  = ../../../

MAKE_MAC = MAKE_MAC_TSCH
MAKE_ROUTING = MAKE_ROUTING_NULLROUTING
MODULES += os/net/mac/tsch/sixtop

all: $(CONTIKI_PROJECT)
include $(CONTIKI)/Makefile.include
