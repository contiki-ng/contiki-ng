CONTIKI_PROJECT = cc26xx-web-demo
all: $(CONTIKI_PROJECT)

PLATFORMS_ONLY = srf06-cc26xx

MODULES_REL += ./resources

PROJECT_SOURCEFILES += cetic-6lbr-client.c coap-server.c net-uart.c mqtt-client.c
PROJECT_SOURCEFILES += httpd-simple.c

# REST Engine shall use Erbium CoAP implementation
MODULES += os/net/app-layer/mqtt
MODULES += os/net/app-layer/coap

CONTIKI=../../../..
include $(CONTIKI)/Makefile.include
