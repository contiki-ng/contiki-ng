CONTIKI_PROJECT = example-ipso-objects
all: $(CONTIKI_PROJECT)

CONTIKI = ../../../..

PLATFORMS_ONLY = nrf

# Reuse the stock LwM2M/IPSO example sources; only the configuration differs.
LWM2M_EXAMPLE = $(CONTIKI)/examples/lwm2m-ipso-objects
PROJECTDIRS += $(LWM2M_EXAMPLE)
CONTIKI_SOURCEFILES += example-ipso-temperature.c

# Our project-conf.h sets the NAT64 server address, then defers to the
# stock basic configuration.
CFLAGS += -DPROJECT_CONF_PATH=\"project-conf.h\"

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap
MODULES += $(CONTIKI_NG_SERVICES_DIR)/lwm2m
MODULES += $(CONTIKI_NG_SERVICES_DIR)/ipso-objects

include $(CONTIKI)/Makefile.include
