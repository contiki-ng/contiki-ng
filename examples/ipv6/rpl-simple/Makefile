all: node
CONTIKI=../../..

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

WITH_TSCH ?= 0

ifeq ($(WITH_TSCH),1)
MAKE_MAC = MAKE_MAC_TSCH
endif
CFLAGS += -DWITH_TSCH=$(WITH_TSCH)

CONTIKI_WITH_IPV6 = 1
CONTIKI_WITH_RPL_LITE = 1
include $(CONTIKI)/Makefile.include
