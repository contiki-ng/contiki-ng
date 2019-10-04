CONTIKI_PROJECT = coap-client
CONTIKI=../../../../..

PLATFORMS_ONLY = nrf52dk

ifeq ($(SERVER_IPV6_EP),)
$(warning Please define SERVER_IPV6_EP=<full-coap-IPv6-URI>)
$(warning Using default SERVER_IPV6_EP=fd00::1)
SERVER_IPV6_EP=fd00::1
endif

CFLAGS += -DSERVER_IPV6_EP=\"$(SERVER_IPV6_EP)\"

CFLAGS += -DDEVICE_NAME=\"nRF52_DK_CoAP_Client\"

CFLAGS += -DCOAP_OBSERVE_CLIENT=1

# automatically build RESTful resources
MODULES_REL += ./resources

# REST Engine shall use Erbium CoAP implementation
include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap

MAKE_ROUTING = MAKE_ROUTING_NULLROUTING

all: $(CONTIKI_PROJECT)

include $(CONTIKI)/Makefile.include
