CONTIKI_PROJECT = border-router
all: $(CONTIKI_PROJECT)
CONTIKI = ../..

include $(CONTIKI)/Makefile.identify-target

ifeq ($(TARGET),native)
  SOURCES_DIR = native
else
  SOURCES_DIR = embedded
endif
include $(SOURCES_DIR)/Makefile

PROJECTDIRS += common
PROJECTDIRS += $(SOURCES_DIR)
CFLAGS += -DPROJECT_CONF_PATH=\"$(SOURCES_DIR)/project-conf.h\"

include $(CONTIKI)/Makefile.include
