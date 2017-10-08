CONTIKI_PROJECT=slip-radio
all: $(CONTIKI_PROJECT)
MODULES += os/services/slip-cmd

CONTIKI=../../..
-include $(CONTIKI)/Makefile.identify-target 

### Optionally, the target can add its own Makefile, to do things like e.g.
### add more source files to the build or define make variables.
-include $(TARGET)/Makefile.$(TARGET)

# Automatically include target-conf.h if found
ifneq ("$(wildcard $(TARGET)/target-conf.h)","")
CFLAGS += -DTARGET_CONF_PATH=\"target-conf.h\"
endif

PROJECTDIRS += $(TARGET)
PROJECT_SOURCEFILES += slip-net.c

# custom net layer, but with IPv6 enabled
MAKE_NET = MAKE_NET_IPV6
CONTIKI_WITH_RPL = 0
include $(CONTIKI)/Makefile.include
