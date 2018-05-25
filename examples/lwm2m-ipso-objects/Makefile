CONTIKI_PROJECT = example-ipso-objects

CONTIKI_SOURCEFILES += serial-protocol.c example-ipso-temperature.c

PLATFORMS_EXCLUDE = sky

all: $(CONTIKI_PROJECT)

MODULES += os/net/app-layer/coap
MODULES += os/services/lwm2m
MODULES += os/services/ipso-objects

CONTIKI=../..
include $(CONTIKI)/Makefile.identify-target
MODULES_REL += $(TARGET)

include $(CONTIKI)/Makefile.include
