CONTIKI_PROJECT = cc26x0-web-demo
all: $(CONTIKI_PROJECT)

PLATFORMS_ONLY = cc26x0-cc13x0

MODULES_REL += ./resources

NEEDS_CONTIKI_VERSION_FILES += res-device.c

PROJECT_SOURCEFILES += coap-server.c net-uart.c mqtt-client.c
PROJECT_SOURCEFILES += httpd-simple.c

ifeq ($(MAKE_ROUTING),MAKE_ROUTING_RPL_CLASSIC)
# 6lbr only supports RPL Classic
PROJECT_SOURCEFILES += cetic-6lbr-client.c
endif

CONTIKI=../../../..

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/mqtt $(CONTIKI_NG_APP_LAYER_DIR)/coap

include $(CONTIKI)/Makefile.include
