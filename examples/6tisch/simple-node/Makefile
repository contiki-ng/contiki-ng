CONTIKI_PROJECT = node
all: $(CONTIKI_PROJECT)

PLATFORMS_EXCLUDE = sky nrf52dk native simplelink
BOARDS_EXCLUDE = srf06/cc13xx launchpad/cc1310 launchpad/cc1350 sensortag/cc2650 sensortag/cc1350

CONTIKI=../../..

 # force Orchestra from command line
MAKE_WITH_ORCHESTRA ?= 0
# force Security from command line
MAKE_WITH_SECURITY ?= 0
 # print #routes periodically, used for regression tests
MAKE_WITH_PERIODIC_ROUTES_PRINT ?= 0

MAKE_MAC = MAKE_MAC_TSCH
MODULES += os/services/shell

ifeq ($(MAKE_WITH_ORCHESTRA),1)
MODULES += os/services/orchestra
endif

ifeq ($(MAKE_WITH_SECURITY),1)
CFLAGS += -DWITH_SECURITY=1
endif

ifeq ($(MAKE_WITH_PERIODIC_ROUTES_PRINT),1)
CFLAGS += -DWITH_PERIODIC_ROUTES_PRINT=1
endif

include $(CONTIKI)/Makefile.include
