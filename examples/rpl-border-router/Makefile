CONTIKI_PROJECT = border-router
all: $(CONTIKI_PROJECT)
CONTIKI = ../..

include $(CONTIKI)/Makefile.identify-target

ifeq ($(TARGET),native)
  PROJECTDIRS += native
	CFLAGS += -DPROJECT_CONF_PATH=\"native/project-conf.h\"
else
	PROJECTDIRS += embedded
	# For embedded platforms, include platform-specific directory
	PROJECTDIRS += embedded/$(TARGET)
	CFLAGS += -DPROJECT_CONF_PATH=\"embedded/project-conf.h\"
endif
PROJECTDIRS += common

include $(CONTIKI)/Makefile.include
