CONTIKI_PROJECT = coap-plugtest-server
all: $(CONTIKI_PROJECT)

# Only intended for native
PLATFORMS_ONLY = native

# Include the CoAP implementation
MODULES += os/net/app-layer/coap

# Include CoAP resources
MODULES_REL += ./resources

CONTIKI=../../..
include $(CONTIKI)/Makefile.include
