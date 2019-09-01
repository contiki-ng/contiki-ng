CONTIKI_PROJECT = mqtt-demo
all: $(CONTIKI_PROJECT)

PLATFORMS_ONLY = nrf52dk

MAKE_ROUTING = MAKE_ROUTING_NULLROUTING

CONTIKI=../../../..

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/mqtt

include $(CONTIKI)/Makefile.include
