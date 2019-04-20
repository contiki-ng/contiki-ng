CONTIKI_PROJECT = nullnet-broadcast nullnet-unicast
all: $(CONTIKI_PROJECT)

CONTIKI = ../../../..

PLATFORMS_ONLY = nrf52840dk sky

NRF52_WITHOUT_SOFTDEVICE=1

WERROR=0

#use this to enable TSCH: MAKE_MAC = MAKE_MAC_TSCH
MAKE_MAC ?= MAKE_MAC_CSMA
MAKE_NET = MAKE_NET_NULLNET
include $(CONTIKI)/Makefile.include
