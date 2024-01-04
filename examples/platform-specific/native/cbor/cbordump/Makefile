CONTIKI_PROJECT = cbordump

all: $(CONTIKI_PROJECT)

PLATFORMS_ONLY = native

CONTIKI = ../../../../..

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_SERVICES_DIR)/cbor
MODULES += $(CONTIKI_NG_SERVICES_DIR)/unit-test

include $(CONTIKI)/Makefile.include
