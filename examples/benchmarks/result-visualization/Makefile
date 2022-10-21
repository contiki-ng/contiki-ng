CONTIKI_PROJECT = node
all: $(CONTIKI_PROJECT)

PLATFORMS_EXCLUDE = sky native

CONTIKI=../../..

MAKE_MAC = MAKE_MAC_TSCH
MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC

# Energy usage estimation
MODULES += os/services/simple-energest

include $(CONTIKI)/Makefile.dir-variables
include $(CONTIKI)/Makefile.include
