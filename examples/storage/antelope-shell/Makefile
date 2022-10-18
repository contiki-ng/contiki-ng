CONTIKI = ../../..

include $(CONTIKI)/Makefile.dir-variables

PLATFORMS_ONLY = cc2538dk native zoul

MODULES += $(CONTIKI_NG_STORAGE_DIR)/antelope

# If the native platform is used, we need to enable Coffee.
MAKE_CFS = MAKE_CFS_COFFEE

CONTIKI_PROJECT = shell-db
all: $(CONTIKI_PROJECT)

include $(CONTIKI)/Makefile.include
