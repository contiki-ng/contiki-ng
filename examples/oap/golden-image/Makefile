CONTIKI_PROJECT = golden-image
all: $(CONTIKI_PROJECT)

MODULES += os/net/app-layer/ota

PLATFORMS_ONLY = srf06-cc26xx cc2538dk openmote-cc2538 zoul

CONTIKI = ../../..
-include $(CONTIKI)/Makefile.identify-target

MODULES_REL += arch/platform/$(TARGET)

include $(CONTIKI)/Makefile.include
