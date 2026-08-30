CONTIKI_PROJECT = nat64-node
all: $(CONTIKI_PROJECT)

CONTIKI = ../../../..

PLATFORMS_ONLY = nrf

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_SERVICES_DIR)/resolv

include $(CONTIKI)/Makefile.include
