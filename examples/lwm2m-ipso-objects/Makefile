CONTIKI_PROJECT = example-ipso-objects

CONTIKI_SOURCEFILES += serial-protocol.c example-ipso-temperature.c

PLATFORMS_EXCLUDE = sky z1

all: $(CONTIKI_PROJECT)

CONTIKI=../..

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap
MODULES += $(CONTIKI_NG_SERVICES_DIR)/lwm2m
MODULES += $(CONTIKI_NG_SERVICES_DIR)/ipso-objects

include $(CONTIKI)/Makefile.identify-target
MODULES_REL += $(TARGET)

include $(CONTIKI)/Makefile.include
