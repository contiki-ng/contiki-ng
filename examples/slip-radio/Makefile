CONTIKI_PROJECT=slip-radio
all: $(CONTIKI_PROJECT)
MODULES += os/services/slip-cmd

CONTIKI=../..
-include $(CONTIKI)/Makefile.identify-target

### Optionally, the target can add its own Makefile, to do things like e.g.
### add more source files to the build or define make variables.
-include $(TARGET)/Makefile.$(TARGET)

PROJECTDIRS += $(TARGET)
PROJECT_SOURCEFILES += slip-net.c

# custom net layer, but with IPv6 enabled
MAKE_NET = MAKE_NET_IPV6
MAKE_ROUTING = MAKE_ROUTING_NONE
include $(CONTIKI)/Makefile.include
