CONTIKI_PROJECT = coap-plugtest-server
all: $(CONTIKI_PROJECT)

# Only intended for native
PLATFORMS_ONLY = native

# Include CoAP resources
MODULES_REL += ./resources

CONTIKI=../../..

# Include the CoAP implementation
include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap

include $(CONTIKI)/Makefile.include
