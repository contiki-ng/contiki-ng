CONTIKI_PROJECT = sensniff
CONTIKI = ../..

PLATFORMS_ONLY = cc2538dk openmote-cc2538 zoul srf06-cc26xx jn516x simplelink

PROJECT_SOURCEFILES += sensniff-mac.c netstack.c
MODULES_REL += pool $(TARGET)

include $(CONTIKI)/Makefile.identify-target

all: $(CONTIKI_PROJECT)

# use a custom MAC driver: sensniff_mac_driver
MAKE_MAC = MAKE_MAC_OTHER
include $(CONTIKI)/Makefile.include

PYTHON ?= python
SENSNIFF = $(CONTIKI)/tools/sensniff/sensniff.py

ifeq ($(BAUDRATE),)
  BAUDRATE = 460800
endif

SENSNIFF_FLAGS += -b $(BAUDRATE)

ifneq ($(PORT),)
  SENSNIFF_FLAGS += -d $(PORT)
endif

sniff:
ifeq ($(wildcard $(SENSNIFF)), )
	$(error Could not find the sensniff script. Did you run 'git submodule update --init' ?")
else
	$(PYTHON) $(SENSNIFF) $(SENSNIFF_FLAGS)
endif
