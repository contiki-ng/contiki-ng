PROJECTDIRS += common
PREFIX ?= fd00::1/64
CONTIKI = ../..

-include $(CONTIKI)/Makefile.identify-target

all: border-router.$(TARGET)

ifeq ($(TARGET),native)
  SOURCES_DIR = native
else
  SOURCES_DIR = embedded
endif

include $(SOURCES_DIR)/Makefile

PROJECTDIRS += $(SOURCES_DIR)
CFLAGS += -DPROJECT_CONF_PATH=\"$(SOURCES_DIR)/project-conf.h\"

include $(CONTIKI)/Makefile.include
