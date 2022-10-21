CONTIKI_PROJECT = root intermediate sink
all: $(CONTIKI_PROJECT)

# does not fit on sky and z1 motes
PLATFORMS_EXCLUDE = sky z1

CONTIKI = ../..

include $(CONTIKI)/Makefile.identify-target
MODULES_REL += $(TARGET)

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_NET_DIR)/ipv6/multicast

MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC
include $(CONTIKI)/Makefile.include
