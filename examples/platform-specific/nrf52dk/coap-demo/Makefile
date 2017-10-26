CONTIKI=../../../..

ifeq ($(MAKECMDGOALS),)
$(error Please specify whether coap-client or coap-server should be built)
endif

ifneq ($(filter coap-client coap-client.flash, $(MAKECMDGOALS)),)
ifeq ($(SERVER_IPV6_EP),)
$(error Please define SERVER_IPV6_EP=<full-coap-IPv6-URI>)
else
CFLAGS += -DSERVER_IPV6_EP=\"$(SERVER_IPV6_EP)\"
CFLAGS += -DDEVICE_NAME=\"nRF52_DK_CoAP_Client\"
endif
else
CFLAGS += -DDEVICE_NAME=\"nRF52-DK-CoAP-Server\"
endif

# automatically build RESTful resources
MODULES_REL += ./resources

# REST Engine shall use Erbium CoAP implementation
MODULES += os/net/app-layer/coap

MAKE_ROUTING = MAKE_ROUTING_NONE

include $(CONTIKI)/Makefile.include
