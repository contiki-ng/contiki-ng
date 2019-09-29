CONTIKI_PROJECT = dongle-node
all: $(CONTIKI_PROJECT)

PLATFORMS_ONLY = jn516x

TARGET ?= jn516x
JN516x_WITH_DONGLE = 1

CONTIKI=../../../../..

MAKE_MAC = MAKE_MAC_TSCH

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_LIB_DIR)/json
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap

MODULES_REL += ..

include $(CONTIKI)/Makefile.include
