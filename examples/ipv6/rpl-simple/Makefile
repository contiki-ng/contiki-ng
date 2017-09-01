all: node
CONTIKI=../../..

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

WITH_TSCH ?= 0

ifeq ($(WITH_TSCH),1)
MODULES += os/net/mac/tsch
endif
CFLAGS += -DWITH_TSCH=$(WITH_TSCH)

CONTIKI_WITH_IPV6 = 1
CONTIKI_WITH_RPL_LITE = 1
include $(CONTIKI)/Makefile.include
