CONTIKI_PROJECT = bootloader
all: $(CONTIKI_PROJECT)

MODULES += os/net/app-layer/ota

PLATFORMS_EXCLUDE = native 

MAKE_NET = MAKE_NET_NULLNET
MAKE_MAC = MAKE_MAC_NULLMAC
MAKE_ROUTING = MAKE_ROUTING_NULLROUTING
SMALL = 1

CONTIKI = ../../..
-include $(CONTIKI)/Makefile.identify-target

-include arch/platform/$(TARGET)/Makefile.$(TARGET)

include $(CONTIKI)/Makefile.include
