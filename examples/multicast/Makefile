CONTIKI_PROJECT = root intermediate sink
all: $(CONTIKI_PROJECT)

# nrf52dk only supports slave mode, i.e., with no routing
PLATFORMS_EXCLUDE = nrf52dk
# does not fit sky motes
PLATFORMS_EXCLUDE += sky

CONTIKI = ../..

include $(CONTIKI)/Makefile.identify-target
MODULES_REL += $(TARGET)

MODULES += os/net/ipv6/multicast
MODULES += os/services/shell

ifeq ($(DISABLE_AUTO_ROOT),1)
	CFLAGS += -DDISABLE_AUTO_ROOT
endif

MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC
include $(CONTIKI)/Makefile.include
