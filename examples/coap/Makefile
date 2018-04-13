CONTIKI_PROJECT = coap-example-server coap-example-client

all: $(CONTIKI_PROJECT)

CONTIKI=../..

# Do not try to build on Sky because of code size limitation
PLATFORMS_EXCLUDE = sky

# Include the CoAP implementation
MODULES += os/net/app-layer/coap

include $(CONTIKI)/Makefile.identify-target
ifeq ($(TARGET),native)
# Include Plugtest resources only for target native, where code size is unconstrained
	MODULES_REL += ./resources-plugtest
endif
# Include CoAP resources
MODULES_REL += ./resources

include $(CONTIKI)/Makefile.include
