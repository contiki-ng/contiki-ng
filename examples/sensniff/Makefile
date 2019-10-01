CONTIKI_PROJECT = sensniff
CONTIKI = ../..

PLATFORMS_ONLY = cc2538dk openmote z1 zoul cc26x0-cc13x0 jn516x simplelink

PROJECT_SOURCEFILES += sensniff-mac.c netstack.c
MODULES_REL += pool $(TARGET)

include $(CONTIKI)/Makefile.identify-target

all: $(CONTIKI_PROJECT)

# Disable all networking layer 3 upwards 
MAKE_NET=MAKE_NET_NULLNET

# use a custom MAC driver: sensniff_mac_driver
MAKE_MAC = MAKE_MAC_OTHER

PYTHON ?= python
SENSNIFF = $(CONTIKI)/tools/sensniff/sensniff.py

ifeq ($(BAUDRATE),)
  BAUDRATE = 460800
endif

SENSNIFF_FLAGS += -b $(BAUDRATE)

ifneq ($(PORT),)
  SENSNIFF_FLAGS += -d $(PORT)
endif

include $(CONTIKI)/Makefile.include

sniff:
ifeq ($(wildcard $(SENSNIFF)), )
	$(error Could not find the sensniff script. Did you run 'git submodule update --init' ?")
else
	$(PYTHON) $(SENSNIFF) $(SENSNIFF_FLAGS)
endif
