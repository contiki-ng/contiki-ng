CONTIKI_PROJECT = node
all: $(CONTIKI_PROJECT)

CONTIKI=../../..

MAKE_WITH_ORCHESTRA ?= 0 # force Orchestra from command line
MAKE_WITH_SECURITY ?= 0 # force Security from command line
MAKE_WITH_PERIODIC_ROUTES_PRINT ?= 0 # print #routes periodically, used for regression tests

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
