CONTIKI_PROJECT = node-sixtop
all: $(CONTIKI_PROJECT)

PLATFORMS_EXCLUDE = sky nrf52dk native simplelink
BOARDS_EXCLUDE = srf06/cc13xx launchpad/cc1310 launchpad/cc1350 sensortag/cc2650 sensortag/cc1350

PROJECT_SOURCEFILES += sf-simple.c
CONTIKI=../../..

MAKE_WITH_SECURITY ?= 0 # force Security from command line

MAKE_MAC = MAKE_MAC_TSCH
MODULES += os/net/mac/tsch/sixtop

ifeq ($(MAKE_WITH_SECURITY),1)
CFLAGS += -DWITH_SECURITY=1
endif

include $(CONTIKI)/Makefile.include
