CONTIKI_PROJECT=slip-radio
all: $(CONTIKI_PROJECT)

# slip-radio is only intended for platforms with SLIP support
PLATFORMS_EXCLUDE = native

# The sky directory overrides the architecture putchar().
CONTIKI_SOURCES_EXCLUDES_SKY += uart-putchar.c

CONTIKI=../..
include $(CONTIKI)/Makefile.identify-target

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_SERVICES_DIR)/slip-cmd

MODULES_REL += $(TARGET)
PROJECT_SOURCEFILES += slip-net.c

# custom net layer, but with IPv6 enabled
MAKE_NET = MAKE_NET_IPV6
MAKE_ROUTING = MAKE_ROUTING_NULLROUTING
include $(CONTIKI)/Makefile.include
