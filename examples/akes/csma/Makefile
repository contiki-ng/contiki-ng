CONTIKI_PROJECT = plain
all: $(CONTIKI_PROJECT)

CONTIKI = ../../..
include $(CONTIKI)/Makefile.dir-variables

MAKE_MAC = MAKE_MAC_CSMA
MODULES += $(CONTIKI_NG_SERVICES_DIR)/akes

include $(CONTIKI)/Makefile.include
