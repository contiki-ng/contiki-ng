CONTIKI_PROJECT = web-demo

PLATFORMS_ONLY = simplelink

all: $(CONTIKI_PROJECT)

MODULES_REL += ./resources

PROJECT_SOURCEFILES += coap-server.c net-uart.c mqtt-client.c
PROJECT_SOURCEFILES += httpd-simple.c

ifeq ($(MAKE_ROUTING),MAKE_ROUTING_RPL_CLASSIC)
# 6lbr only supports RPL Classic
PROJECT_SOURCEFILES += cetic-6lbr-client.c
endif

# REST Engine shall use Erbium CoAP implementation
MODULES += os/net/app-layer/mqtt
MODULES += os/net/app-layer/coap
MODULES += arch/dev/ext-flash

CONTIKI = ../../../../..
include $(CONTIKI)/Makefile.include
