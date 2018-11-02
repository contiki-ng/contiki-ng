CONTIKI_PROJECT=client

all: $(CONTIKI_PROJECT)

PLATFORMS_ONLY = cc26x0-cc13x0
BOARDS_ONLY = launchpad/cc2650 sensortag/cc2650 srf06/cc26x0

MAKE_MAC = MAKE_MAC_BLE
MAKE_NET = MAKE_NET_IPV6
CONTIKI = ../../../..
include $(CONTIKI)/Makefile.include
