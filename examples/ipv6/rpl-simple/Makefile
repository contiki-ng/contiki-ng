all: node
CONTIKI=../../..

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

# Set MAKE_MAC = MAKE_MAC_TSCH to run TSCH instead
MAKE_MAC = MAKE_MAC_CSMA

CONTIKI_WITH_IPV6 = 1
CONTIKI_WITH_RPL_LITE = 1
include $(CONTIKI)/Makefile.include
