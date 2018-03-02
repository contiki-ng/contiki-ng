CONTIKI_PROJECT = coap-server
CONTIKI=../../../../..

PLATFORMS_ONLY = nrf52dk

CFLAGS += -DDEVICE_NAME=\"nRF52-DK-CoAP-Server\"

# automatically build RESTful resources
REST_RESOURCES_DIR = ../resources
REST_RESOURCES_FILES = $(notdir $(shell find $(REST_RESOURCES_DIR) -name '*.c' ! -name 'res-plugtest*'))

PROJECTDIRS += $(REST_RESOURCES_DIR)
PROJECT_SOURCEFILES += $(REST_RESOURCES_FILES)

# REST Engine shall use Erbium CoAP implementation
MODULES += os/net/app-layer/coap

MAKE_ROUTING = MAKE_ROUTING_NULLROUTING

all: $(CONTIKI_PROJECT)

include $(CONTIKI)/Makefile.include
