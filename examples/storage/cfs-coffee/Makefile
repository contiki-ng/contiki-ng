CONTIKI = ../../..

PLATFORMS_ONLY = cc2538dk native sky zoul gecko

include $(CONTIKI)/Makefile.dir-variables

MODULES += $(CONTIKI_NG_STORAGE_DIR)/cfs $(CONTIKI_NG_SERVICES_DIR)/unit-test

# If the native platform is used, we need to enable Coffee.
MAKE_CFS = MAKE_CFS_COFFEE

CONTIKI_PROJECT = test-cfs example-coffee
all: $(CONTIKI_PROJECT)

include $(CONTIKI)/Makefile.include
