CONTIKI_PROJECT = border-router
all: $(CONTIKI_PROJECT)
CONTIKI = ../..

include $(CONTIKI)/Makefile.identify-target

ifeq ($(TARGET),native)
  MODULES_REL += native
	CFLAGS += -DPROJECT_CONF_PATH=\"native/project-conf.h\"
else
	MODULES_REL += embedded
	# For embedded platforms, include platform-specific directory
	MODULES_REL += embedded/$(TARGET)
	CFLAGS += -DPROJECT_CONF_PATH=\"embedded/project-conf.h\"
endif
MODULES_REL += common

include $(CONTIKI)/Makefile.include
