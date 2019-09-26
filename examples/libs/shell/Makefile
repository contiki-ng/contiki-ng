CONTIKI_PROJECT = example
all: $(CONTIKI_PROJECT)

CONTIKI = ../../..

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_SERVICES_DIR)/shell

PLATFORMS_EXCLUDE = sky

include $(CONTIKI)/Makefile.include
