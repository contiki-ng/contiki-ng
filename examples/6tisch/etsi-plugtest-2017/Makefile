CONTIKI_PROJECT = node
all: $(CONTIKI_PROJECT)

PLATFORMS_EXCLUDE = sky nrf52dk native simplelink
BOARDS_EXCLUDE = srf06/cc13xx launchpad/cc1310 launchpad/cc1350 sensortag/cc2650 sensortag/cc1350

MAKE_WITH_SECURITY   ?= 0 # force Security from command line
ifeq ($(MAKE_WITH_SECURITY),1)
CFLAGS += -DWITH_SECURITY=1
endif

MODULES             += os/services/shell
MODULES             += os/net/mac/tsch/sixtop
PROJECT_SOURCEFILES += sf-plugtest.c
CFLAGS              += -Wno-nonnull-compare

ifeq ($(BOARD),launchpad/cc2650)
# Enable DAP and TAP by default for ETSI Plugtest
CFLAGS += -DSET_CCFG_CCFG_TAP_DAP_0_CPU_DAP_ENABLE=0xC5
CFLAGS += -DSET_CCFG_CCFG_TAP_DAP_0_TEST_TAP_ENABLE=0xC5
endif

MAKE_MAC = MAKE_MAC_TSCH

CONTIKI              =../../..
include $(CONTIKI)/Makefile.include
