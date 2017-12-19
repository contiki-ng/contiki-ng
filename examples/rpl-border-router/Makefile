CONTIKI_PROJECT = border-router
all: $(CONTIKI_PROJECT)
CONTIKI = ../..

include $(CONTIKI)/Makefile.identify-target

ifneq ($(TARGET),native)
	MODULES_REL += embedded
endif
MODULES_REL += $(TARGET)
MODULES_REL += common

include $(CONTIKI)/Makefile.include
