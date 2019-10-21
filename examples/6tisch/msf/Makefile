CONTIKI_PROJECT = msf-node msf-root
all: $(CONTIKI_PROJECT)

PLATFORMS_EXCLUDE = sky z1 nrf52dk native simplelink

CONTIKI = ../../..

MAKE_MAC = MAKE_MAC_TSCH
MODULES += os/net/mac/tsch/sixtop
MODULES += os/services/msf

include $(CONTIKI)/Makefile.include
