CONTIKI_PROJECT = json2cbor

all: $(CONTIKI_PROJECT)

PLATFORMS_ONLY = native

CONTIKI = ../../../../..

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_SERVICES_DIR)/cbor
MODULES += $(CONTIKI_NG_SERVICES_DIR)/unit-test

LDLIBS += -lcjson

include $(CONTIKI)/Makefile.include
