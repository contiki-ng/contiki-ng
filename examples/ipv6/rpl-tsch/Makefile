CONTIKI_PROJECT = node
all: $(CONTIKI_PROJECT)

CONTIKI=../../..
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

CONTIKI_WITH_IPV6 = 1
MAKE_WITH_ORCHESTRA ?= 0 # force Orchestra from command line
MAKE_WITH_SECURITY ?= 0 # force Security from command line
MAKE_WITH_PERIODIC_ROUTES_PRINT ?= 0 # print #routes periodically, used for regression tests

APPS += orchestra shell
MODULES += os/net/mac/tsch

ifeq ($(MAKE_WITH_ORCHESTRA),1)
CFLAGS += -DWITH_ORCHESTRA=1
endif

ifeq ($(MAKE_WITH_SECURITY),1)
CFLAGS += -DWITH_SECURITY=1
endif

ifeq ($(MAKE_WITH_PERIODIC_ROUTES_PRINT),1)
CFLAGS += -DWITH_PERIODIC_ROUTES_PRINT=1
endif

include $(CONTIKI)/Makefile.include
