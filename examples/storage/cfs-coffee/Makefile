CONTIKI = ../../..

PLATFORMS_ONLY= cc2538dk zoul sky

include $(CONTIKI)/Makefile.dir-variables

MODULES += $(CONTIKI_NG_STORAGE_DIR)/cfs $(CONTIKI_NG_SERVICES_DIR)/unit-test

CONTIKI_PROJECT = test-cfs test-coffee example-coffee
all: $(CONTIKI_PROJECT)

include $(CONTIKI)/Makefile.include
