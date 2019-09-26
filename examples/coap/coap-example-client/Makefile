CONTIKI_PROJECT = coap-example-client
# coap-example-observe-client is outdated but will be ported at a later point
all: $(CONTIKI_PROJECT)

# Do not try to build on Sky because of code size limitation
PLATFORMS_EXCLUDE = sky z1

CONTIKI=../../..

# Include the CoAP implementation
include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap

include $(CONTIKI)/Makefile.include
