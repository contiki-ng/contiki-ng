CONTIKI_PROJECT = golden-image
all: $(CONTIKI_PROJECT)

PLATFORMS_EXCLUDE = native 

CONTIKI = ../../..
-include $(CONTIKI)/Makefile.identify-target

MODULES_REL += arch/platform/$(TARGET)

include $(CONTIKI)/Makefile.include
