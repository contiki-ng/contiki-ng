CONTIKI = ../../..

include $(CONTIKI)/Makefile.dir-variables

MODULES += $(CONTIKI_NG_STORAGE_DIR)/antelope $(CONTIKI_NG_SERVICES_DIR)/unit-test

# does not fit on Sky
PLATFORMS_ONLY= cc2538dk zoul

CONTIKI_PROJECT = shell-db
all: $(CONTIKI_PROJECT)

include $(CONTIKI)/Makefile.include
