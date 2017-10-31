PROJECTDIRS += common

CONTIKI=../..

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

border-router.$(TARGET): $(SOURCES_DIR)/border-router.$(TARGET)
	cp $< $@

CLEAN += border-router.$(TARGET) $(SOURCES_DIR)/border-router.$(TARGET)

ifeq ($(PREFIX),)
  PREFIX = fd00::1/64
endif

include $(CONTIKI)/Makefile.include
